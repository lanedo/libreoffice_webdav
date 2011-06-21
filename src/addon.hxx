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

#ifndef _Addon_HXX
#define _Addon_HXX

#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <cppuhelper/implbase4.hxx>

namespace css = com::sun::star;

namespace com
{
    namespace sun
    {
        namespace star
        {
            namespace frame
            {
                class XFrame;
            }
        }
    }
}

#define IMPLEMENTATION_NAME "com.lanedo.webdavui"

class Addon : public cppu::WeakImplHelper4
<
    css::frame::XDispatchProvider,
    css::frame::XDispatch,
    css::lang::XInitialization,
    css::lang::XServiceInfo
>
{
private:
    css::uno::Reference< css::lang::XMultiServiceFactory > mxMSF;
    css::uno::Reference< css::frame::XFrame > mxFrame;

public:
    Addon( const css::uno::Reference< css::lang::XMultiServiceFactory > &rxMSF)
        : mxMSF( rxMSF ) {}

    // XDispatchProvider
    virtual css::uno::Reference< css::frame::XDispatch >
            SAL_CALL queryDispatch(	const css::util::URL& aURL,
                const rtl::OUString& sTargetFrameName, sal_Int32 nSearchFlags )
                throw( css::uno::RuntimeException );
    virtual css::uno::Sequence < css::uno::Reference< css::frame::XDispatch > >
        SAL_CALL queryDispatches(
            const css::uno::Sequence < css::frame::DispatchDescriptor >& seqDescriptor )
            throw( css::uno::RuntimeException );

    // XDispatch
    virtual void SAL_CALL dispatch( const css::util::URL& aURL,
        const css::uno::Sequence< css::beans::PropertyValue >& lArgs )
        throw (css::uno::RuntimeException);
    virtual void SAL_CALL addStatusListener( const css::uno::Reference< css::frame::XStatusListener >& xControl,
        const css::util::URL& aURL ) throw (css::uno::RuntimeException);
    virtual void SAL_CALL removeStatusListener( const css::uno::Reference< css::frame::XStatusListener >& xControl,
        const css::util::URL& aURL ) throw (css::uno::RuntimeException);

    // XInitialization
    virtual void SAL_CALL initialize( const css::uno::Sequence< css::uno::Any >& aArguments )
        throw (css::uno::Exception, css::uno::RuntimeException);

    // XServiceInfo
    virtual rtl::OUString SAL_CALL getImplementationName(  )
        throw (css::uno::RuntimeException);
    virtual sal_Bool SAL_CALL supportsService( const rtl::OUString& ServiceName )
        throw (css::uno::RuntimeException);
    virtual css::uno::Sequence< rtl::OUString > SAL_CALL getSupportedServiceNames(  )
        throw (css::uno::RuntimeException);
};

rtl::OUString Addon_getImplementationName()
    throw ( css::uno::RuntimeException );

sal_Bool SAL_CALL Addon_supportsService( const rtl::OUString& ServiceName )
    throw ( css::uno::RuntimeException );

css::uno::Sequence< rtl::OUString > SAL_CALL Addon_getSupportedServiceNames(  )
    throw ( css::uno::RuntimeException );

css::uno::Reference< css::uno::XInterface >
SAL_CALL Addon_createInstance( const css::uno::Reference< css::lang::XMultiServiceFactory > & rSMgr)
    throw ( css::uno::Exception );

#endif // _Addon_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
