/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*************************************************************************
 *
 *  Copyright (C) 2011
 *  Michael Natterer <michael.natterer@lanedo.com>
 *  Kristian Rietveld <kris@lanedo.com>
 *  Christian Dywan <christian@lanedo.com>
 *  Lionel Dricot <lionel.dricot@lanedo.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General
 *  Public License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA 
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *************************************************************************/

#include "addon.hxx"
#include "filedialog.hxx"
#include "configdialog.hxx"

#include <osl/diagnose.h>
#include <rtl/ustring.hxx>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <cstdio> // TEMPORARY: for puts

using rtl::OUString;
using namespace css::uno;
using namespace css::frame;
using namespace css::awt;
using css::lang::XMultiComponentFactory;
using css::beans::PropertyValue;
using css::util::URL;

// This is the service name an Add-On has to implement
#define SERVICE_NAME "com.sun.star.frame.ProtocolHandler"


/**
  * Called by the Office framework.
  * One-time initialization. We have to store the context information
  * given, like the frame we are bound to, into our members.
  */
void SAL_CALL Addon::initialize( const Sequence< Any >& aArguments )
    throw (Exception, RuntimeException)
{
    puts ("Addon class initialized");

    Reference < XFrame > xFrame;
    if ( aArguments.getLength() )
    {
        aArguments[0] >>= xFrame;
        mxFrame = xFrame;
    }
}

/**
  * Called by the Office framework.
  * We are ask to query the given URL and return a dispatch object if the URL
  * contains an Add-On command.
  */
Reference< XDispatch > SAL_CALL Addon::queryDispatch( const URL&      aURL,
                                                      const OUString& sTargetFrameName,
                                                      sal_Int32       nSearchFlags )
    throw (RuntimeException)
{
    Reference < XDispatch > xRet;

    if ( aURL.Protocol.compareToAscii("com.lanedo.webdavui:") == 0 )
    {
        xRet = this;
    }

    return xRet;
}

/**
  * Called by the Office framework.
  * We are ask to execute the given Add-On command URL.
  */
void SAL_CALL Addon::dispatch( const URL&                        aURL,
                               const Sequence < PropertyValue >& lArgs )
    throw (RuntimeException)
{
    puts ("dispatch!");

    mSettings = new WebDAVUI::Settings (mxContext);

    if ( aURL.Path.compareToAscii( "configure" ) == 0 )
    {
        puts ("configure selected");
        WebDAVUI::ConfigDialog *dialog = new WebDAVUI::ConfigDialog (mxContext, mxFrame, mSettings);
        dialog->show ();
    }
    else if ( aURL.Path.compareToAscii( "open" ) == 0 )
    {
        puts ("open selected");
        WebDAVUI::FileDialog *dialog = new WebDAVUI::FileDialog (mxContext, mxFrame, mSettings, sal_False);
        dialog->show ();
    }
    else if ( aURL.Path.compareToAscii( "save" ) == 0 )
    {
        puts ("save selected");
        WebDAVUI::FileDialog *dialog = new WebDAVUI::FileDialog (mxContext, mxFrame, mSettings, sal_True);
        dialog->show ();
    }
}

/**
  * Called by the Office framework.
  * We are ask to query the given sequence of URLs and return dispatch objects if the URLs
  * contain Add-On commands.
  */
Sequence < Reference< XDispatch > > SAL_CALL Addon::queryDispatches( const Sequence < DispatchDescriptor >& seqDescripts )
    throw (RuntimeException)
{
    sal_Int32 nCount = seqDescripts.getLength();
    Sequence < Reference < XDispatch > > lDispatcher( nCount );

    for( sal_Int32 i=0; i<nCount; ++i )
        lDispatcher[i] = queryDispatch( seqDescripts[i].FeatureURL,
                                        seqDescripts[i].FrameName,
                                        seqDescripts[i].SearchFlags );

    return lDispatcher;
}

/**
  * Called by the Office framework.
  * We are ask to query the given sequence of URLs and return dispatch objects if the URLs
  * contain Add-On commands.
  */
void SAL_CALL Addon::addStatusListener( const Reference< XStatusListener >& xControl,
                                        const URL&                          aURL )
    throw (RuntimeException)
{
    OUString label;
    sal_Bool sensitive = false;

    puts ("addStatusListener!");

    if ( aURL.Path.compareToAscii( "configure" ) == 0 )
    {
        puts ("addStatusListener(configure)");
        sensitive = true;
    }
    else if ( aURL.Path.compareToAscii( "open" ) == 0 )
    {
        puts ("addStatusListener(open)");
        sensitive = true;
    }
    else if ( aURL.Path.compareToAscii( "save" ) == 0 )
    {
        puts ("addStatusListener(save)");

        if ( mxFrame.is() &&
             mxFrame->getController().is() &&
             mxFrame->getController()->getModel().is() )
        {
            sensitive = true;
        }
    }

    FeatureStateEvent aEvent( static_cast<cppu::OWeakObject*>( this ),
                              aURL,
                              OUString::createFromAscii (""),
                              sensitive,
                              false,
                              Any() );

    xControl->statusChanged( aEvent );
}

/**
  * Called by the Office framework.
  * We are ask to query the given sequence of URLs and return dispatch objects if the URLs
  * contain Add-On commands.
  */
void SAL_CALL Addon::removeStatusListener( const Reference< XStatusListener >& xControl,
                                           const URL&                          aURL )
    throw (RuntimeException)
{
}

//##################################################################################################
//#### Helper functions for the implementation of UNO component interfaces #########################
//##################################################################################################

OUString Addon_getImplementationName()
    throw (RuntimeException)
{
    return OUString ( RTL_CONSTASCII_USTRINGPARAM ( IMPLEMENTATION_NAME ) );
}

sal_Bool SAL_CALL Addon_supportsService( const OUString& ServiceName )
    throw (RuntimeException)
{
    return ServiceName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM ( SERVICE_NAME ) );
}

Sequence< OUString > SAL_CALL Addon_getSupportedServiceNames()
    throw (RuntimeException)
{
    puts ("In Addon_getSupportedServiceNames()");

    Sequence < OUString > aRet(1);
    OUString* pArray = aRet.getArray();

    pArray[0] =  OUString ( RTL_CONSTASCII_USTRINGPARAM ( SERVICE_NAME ) );

    return aRet;
}

Reference< XInterface > SAL_CALL Addon_createInstance( const Reference< XComponentContext > & rContext)
    throw (Exception)
{
    puts ("In Addon_createInstance");
    return (cppu::OWeakObject*) new Addon( rContext );
}

//##################################################################################################
//#### Implementation of the recommended/mandatory interfaces of a UNO component ###################
//##################################################################################################

// XServiceInfo
OUString SAL_CALL Addon::getImplementationName()
    throw (RuntimeException)
{
    return Addon_getImplementationName();
}

sal_Bool SAL_CALL Addon::supportsService( const OUString& rServiceName )
    throw (RuntimeException)
{
    return Addon_supportsService( rServiceName );
}

Sequence< OUString > SAL_CALL Addon::getSupportedServiceNames()
    throw (RuntimeException)
{
    return Addon_getSupportedServiceNames();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
