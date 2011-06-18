/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*************************************************************************
 *
 *  The Contents of this file are made available subject to the terms of
 *  the BSD license.
 *
 *  Copyright 2000, 2010 Oracle and/or its affiliates.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Sun Microsystems, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#include "webdavdialog.hxx"

#include <osl/diagnose.h>
#include <rtl/ustring.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/frame/XFrame.hpp>

#include <cstdio> // TEMPORARY: for puts

using rtl::OUString;
using namespace com::sun::star::uno;
using namespace com::sun::star::frame;
using namespace com::sun::star::awt;
using com::sun::star::lang::XMultiServiceFactory;
using com::sun::star::beans::PropertyValue;
using com::sun::star::util::URL;

// This is the service name an Add-On has to implement
#define SERVICE_NAME "com.sun.star.frame.ProtocolHandler"


/**
  * Called by the Office framework.
  * One-time initialization. We have to store the context information
  * given, like the frame we are bound to, into our members.
  */
void SAL_CALL Addon::initialize( const Sequence< Any >& aArguments ) throw ( Exception, RuntimeException)
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
Reference< XDispatch > SAL_CALL Addon::queryDispatch( const URL&             aURL,
                                                      const ::rtl::OUString& sTargetFrameName,
                                                      sal_Int32              nSearchFlags )
    throw (RuntimeException)
{
    Reference < XDispatch > xRet;

    if ( aURL.Protocol.compareToAscii("com.lanedo.webdavui:") == 0 )
    {
        if ( aURL.Path.compareToAscii( "TestAction" ) == 0 )
            xRet = this;
        else if ( aURL.Path.compareToAscii( "TestAction2" ) == 0 )
            xRet = this;
        else if ( aURL.Path.compareToAscii( "Help" ) == 0 )
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

    if ( aURL.Protocol.compareToAscii("com.lanedo.webdavui:") == 0 )
    {
        if ( aURL.Path.compareToAscii( "TestAction" ) == 0 )
        {
            puts ("TestAction");
            WebDAVDialog *dialog = new WebDAVDialog (mxMSF, mxFrame);
            dialog->show ();
        }
        else if ( aURL.Path.compareToAscii( "TestAction2" ) == 0 )
        {
            puts ("TestAction2");
        }
        else if ( aURL.Path.compareToAscii( "Help" ) == 0 )
        {
            puts ("Help");
        }
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

::rtl::OUString Addon_getImplementationName()
    throw (RuntimeException)
{
    return ::rtl::OUString ( RTL_CONSTASCII_USTRINGPARAM ( IMPLEMENTATION_NAME ) );
}

sal_Bool SAL_CALL Addon_supportsService( const ::rtl::OUString& ServiceName )
    throw (RuntimeException)
{
    return ServiceName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM ( SERVICE_NAME ) );
}

Sequence< ::rtl::OUString > SAL_CALL Addon_getSupportedServiceNames()
    throw (RuntimeException)
{
    puts ("In Addon_getSupportedServiceNames()");

    Sequence < ::rtl::OUString > aRet(1);
    ::rtl::OUString* pArray = aRet.getArray();

    pArray[0] =  ::rtl::OUString ( RTL_CONSTASCII_USTRINGPARAM ( SERVICE_NAME ) );

    return aRet;
}

Reference< XInterface > SAL_CALL Addon_createInstance( const Reference< XMultiServiceFactory > & rSMgr)
    throw (Exception)
{
    puts ("In Addon_createInstance");
    return (cppu::OWeakObject*) new Addon( rSMgr );
}

//##################################################################################################
//#### Implementation of the recommended/mandatory interfaces of a UNO component ###################
//##################################################################################################

// XServiceInfo
::rtl::OUString SAL_CALL Addon::getImplementationName()
    throw (RuntimeException)
{
    return Addon_getImplementationName();
}

sal_Bool SAL_CALL Addon::supportsService( const ::rtl::OUString& rServiceName )
    throw (RuntimeException)
{
    return Addon_supportsService( rServiceName );
}

Sequence< ::rtl::OUString > SAL_CALL Addon::getSupportedServiceNames()
    throw (RuntimeException)
{
    return Addon_getSupportedServiceNames();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
