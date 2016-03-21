/*
    The MIT License (MIT)

    Copyright (c) 2016 Benoit AUTHEMAN

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

//-----------------------------------------------------------------------------
// This file is a part of the GTpo software library.
//
// \file	gtpoSerializer.h
// \author	benoit@qanava.org
// \date	2016 01 31
//-----------------------------------------------------------------------------

#ifndef gtpoSerializer_h
#define gtpoSerializer_h

// GTpo headers
#include "./gtpoUtils.h"
#include "./gtpoGraph.h"
#include "./gtpoProgressNotifier.h"

// STD headers
#include <fstream>
#include <atomic>

namespace gtpo { // ::gtpo

/*! Default interface for a GTpo output serializer.
 *
 */
template < class GraphConfig = gtpo::DefaultConfig >
class OutSerializer {
public:
    using Graph         = const gtpo::GenGraph< GraphConfig >;
    using SharedNode    = std::shared_ptr< typename GraphConfig::Node >;
    using Node          = typename GraphConfig::Node;
    using WeakNode      = std::weak_ptr< typename GraphConfig::Node >;
    using WeakEdge      = std::weak_ptr< typename GraphConfig::Edge >;
    using SharedEdge    = std::shared_ptr< typename GraphConfig::Edge >;

    OutSerializer() = default;
    virtual ~OutSerializer() = default;
    OutSerializer( const OutSerializer< GraphConfig >& ) = delete;

    /*! Serialize \c graph out.
     *
     * \throw std::exception if an error occurs.
     */
    virtual void    serializeOut( Graph& graph, gtpo::IProgressNotifier* progress = nullptr ) noexcept( false ) { (void)graph; (void)progress;}
};

/*! Default interface for a GTpo output serializer.
 *
 */
template < class GraphConfig = DefaultConfig >
class InSerializer {
public:
    using Graph = GenGraph< GraphConfig >;
    using Node  = typename GraphConfig::Node;
    using Edge  = typename GraphConfig::Edge;

    InSerializer() = default;
    virtual ~InSerializer() = default;
    InSerializer( const InSerializer< GraphConfig >& ) = delete;

    /*! Serialize \c graph in.
     *
     * \throw std::exception if an error occurs, graph might be let in an "invalid state" and should no longer be used.
     */
    virtual void    serializeIn( Graph& graph, gtpo::IProgressNotifier* progress = nullptr ) noexcept( false ) { ( void )graph; (void)progress; }
};

/*! Default interface for a GTpo input/output serializer.
 *
 */
template < class GraphConfig = DefaultConfig >
class Serializer :  public InSerializer< GraphConfig >,
                    public OutSerializer< GraphConfig >
{
public:
    using Graph         = GenGraph< GraphConfig >;
    using SharedNode    = std::shared_ptr< typename GraphConfig::Node >;
    using Node          = typename GraphConfig::Node;
    using WeakNode      = std::weak_ptr< typename GraphConfig::Node >;
    using WeakEdge      = std::weak_ptr< typename GraphConfig::Edge >;
    using SharedEdge    = std::shared_ptr< typename GraphConfig::Edge >;

    Serializer() :
        _progressNotifier{ new IProgressNotifier( ) }
    {

    }
    virtual ~Serializer() = default;
    Serializer( const Serializer< GraphConfig >& ) = delete;

public:
    /*! Register a progress notifier in this serializer.
     *
     * \code
     * ConcreteSerializer serializer;
     * serializer.registerProgressNotifier( std::make_unique<gtpo::ProgressNotifier>( ) );  // Avoid any eventual leak if an exception is thrown
     * \endcode
     *
     * \arg progressNotifier progress notifier that should be used by this serializer, serializer get ownership for the notifier.
     * \note Any previously registered notifier is destroyed.
     */
    auto            registerProgressNotifier( std::unique_ptr< gtpo::IProgressNotifier > progressNotifier ) -> void {
        if ( progressNotifier == nullptr )
            return;
        _progressNotifier.swap( progressNotifier );
    }
    /*! \copybrief registerProgressNotifier()
     *
     * Preferred way to register a new progress notifier is by using the sink method with unique_ptr.
     */
    auto            registerProgressNotifier( gtpo::IProgressNotifier* progressNotifier ) -> void {
        if ( progressNotifier == nullptr )
            return;
        _progressNotifier.swap( std::unique_ptr< gtpo::IProgressNotifier >( progressNotifier ) );
    }
    /*! Return this serializer currently registered progress notifier.
     *
     * \note Even if a custom notifier has not been installed with registerProgressNotifier(), this method will never
     * return nullptr (it's default value is a a void IProgressNotifier interface).
     */
    auto            getProgressNotifier() -> gtpo::IProgressNotifier* {
        gtpo::assert_throw( _progressNotifier == nullptr, "gtpo::Serializer<>::getProgressNotifier(): Internal error: serializer progress notifier can't be nullptr." );
        return _progressNotifier.get();
    }
    //! Const shortcut to getProgressNotifier().
    auto            getProgressNotifier() const -> const gtpo::IProgressNotifier* { return _progressNotifier.get(); }

private:
    std::unique_ptr< gtpo::IProgressNotifier >  _progressNotifier;

public:
    /*! Serialize \c graph out.
     *
     * \throw std::exception if an error occurs.
     */
    virtual auto    serializeOut( const Graph& graph, std::ostream& os ) -> void { (void)graph; (void)os; }
    //! Shortcut to serializeOut( const Graph&, std::ostream& ) with a file name instead of an output stream.
    virtual auto    serializeOutTo( const Graph& graph, std::string fileName ) -> void {
        std::ofstream os( fileName, std::ios::out | std::ios::trunc | std::ios::binary );
        if ( !os ) {
            std::cerr << "gtpo::ProtoSerializer::serializeOut(): Error: Can't open output stream " << fileName << std::endl;
            return;
        }
        serializeOut( graph, os );
        if ( os.is_open() )
            os.close();
    }

    /*! Serialize \c graph in from stream \c is.
     *
     * \throw std::exception if an error occurs, graph might be let in an "invalid state" and should no longer be used.
     */
    virtual auto    serializeIn( std::istream& is, Graph& graph ) noexcept( false )  -> void { (void)is; (void)graph; }
    //! Shortcut to serializeIn( std::istream&, Graph& ) with a file name instead of an input stream.
    virtual auto    serializeInFrom( std::string fileName, Graph& graph ) noexcept( false ) -> void {
        std::ifstream is( fileName, std::ios::in | std::ios::binary );
        if ( !is ) {
            std::cerr << "gtpo::ProtoSerializer::serializeIn(): Error: Can't open input stream " << fileName << std::endl;
            return;
        }
        serializeIn( is, graph );
        if ( is.is_open() )
            is.close();
    }
};

} // ::gtpo

#endif // gtpoSerializer_h

