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

// MARKER(update_precomp.py): autogen include statement, do not remove

#include <com/sun/star/frame/XDispatch.hpp>
#include <com/sun/star/frame/XDispatchProviderInterception.hpp>
#include <com/sun/star/frame/XDispatchProviderInterceptor.hpp>
//#include <comphelper/processfactory.hxx>
//#include <comphelper/stlunosequence.hxx>
#include <cppuhelper/implbase1.hxx>
#include <cppuhelper/implbase2.hxx>
#include "webdavuiinterceptor.hxx"
#include <osl/diagnose.h>
#include <stdio.h>

#include <algorithm>

namespace css = com::sun::star;

//using comphelper::getProcessServiceFactory;
//using comphelper::stl_begin;
//using comphelper::stl_end;
using css::beans::PropertyValue;
using css::frame::DispatchDescriptor;
using css::frame::FeatureStateEvent;
using css::frame::XDispatch;
using css::frame::XDispatchProvider;
using css::frame::XDispatchProviderInterception;
using css::frame::XDispatchProviderInterceptor;
using css::frame::XStatusListener;
using css::uno::Any;
using css::uno::makeAny;
using css::uno::Reference;
using css::uno::RuntimeException;
using css::uno::Sequence;
using css::util::URL;
using rtl::OUString;

namespace com { namespace lanedo { namespace webdavui
{
    class WebdavuiDispatch : public ::cppu::WeakImplHelper1< XDispatch >
    {
        public:
            virtual void SAL_CALL dispatch(const URL& aUrl, const Sequence< PropertyValue >& aArgs) throw (RuntimeException);
            virtual void SAL_CALL addStatusListener(const Reference< XStatusListener >&, const URL&) throw (RuntimeException);
            virtual void SAL_CALL removeStatusListener(const Reference< XStatusListener >&, const URL&) throw (RuntimeException)
                {};
            static const OUString our_sTestAction;
            static const OUString our_sTestActionLabel;
    };

    class WebdavuiInterceptor :
        public ::cppu::WeakImplHelper1< XDispatchProviderInterceptor >
    {
        public:
            virtual Reference< XDispatch > SAL_CALL queryDispatch(const URL& aUrl, const OUString& sTargetFrameName, sal_Int32 nSearchFlags) throw (RuntimeException);
            virtual Sequence< Reference< XDispatch > > SAL_CALL queryDispatches(const Sequence< DispatchDescriptor >& lDescriptor) throw (RuntimeException);
            virtual Reference< XDispatchProvider > SAL_CALL getMasterDispatchProvider() throw(RuntimeException)
                { return m_xMaster; };
            virtual Reference< XDispatchProvider > SAL_CALL getSlaveDispatchProvider() throw(RuntimeException)
                { return m_xSlave; };
            virtual void SAL_CALL setMasterDispatchProvider(const Reference< XDispatchProvider >& xMaster) throw(RuntimeException)
                { m_xMaster = xMaster; };
            virtual void SAL_CALL setSlaveDispatchProvider(const Reference< XDispatchProvider >& xSlave) throw(RuntimeException)
                { m_xSlave = xSlave; };
        private:
            Reference< XDispatchProvider > m_xMaster;
            Reference< XDispatchProvider > m_xSlave;
            static const OUString our_sProtocol;
    };

    struct QueryTransformer
    {
        WebdavuiInterceptor* const m_pInterceptor;
        QueryTransformer(WebdavuiInterceptor* pInterceptor) : m_pInterceptor(pInterceptor) {};
        Reference< XDispatch > operator()(const DispatchDescriptor& rDescriptor)
        {
            return m_pInterceptor->queryDispatch(rDescriptor.FeatureURL, OUString(), 0);
        };
    };
}}}

using namespace com::lanedo::webdavui;


const OUString WebdavuiDispatch::our_sTestAction = OUString(RTL_CONSTASCII_USTRINGPARAM("com.lanedo.webdavui:HelpOnline"));
const OUString WebdavuiDispatch::our_sTestActionLabel = OUString(RTL_CONSTASCII_USTRINGPARAM("Test Action Label"));

const OUString WebdavuiInterceptor::our_sProtocol = OUString(RTL_CONSTASCII_USTRINGPARAM("com.lanedo.webdavui:"));


Reference< XDispatch > SAL_CALL WebdavuiInterceptor::queryDispatch(const URL& aUrl, const OUString& sTargetFrameName, sal_Int32 nSearchFlags)
    throw (RuntimeException)
{
    OSL_TRACE("queryDispatch: %s", OUStringToOString(aUrl.Complete, RTL_TEXTENCODING_UTF8).getStr());
    if(aUrl.Protocol.equals(our_sProtocol))
    {
        if(aUrl.Complete.equals(WebdavuiDispatch::our_sTestAction))
            return Reference< XDispatch >(new WebdavuiDispatch());
    }
    return m_xSlave->queryDispatch(aUrl, sTargetFrameName, nSearchFlags);
};

Sequence< Reference< XDispatch > > WebdavuiInterceptor::queryDispatches(const Sequence< DispatchDescriptor >& lDescriptor)
    throw (RuntimeException)
{
    Sequence< Reference< XDispatch > > vResult(lDescriptor.getLength());

    /*
    ::std::transform(
        stl_begin(lDescriptor),
        stl_end(lDescriptor),
        stl_begin(vResult),
        QueryTransformer(this));
    */

    return vResult;
}

void WebdavuiDispatch::dispatch(const URL& aUrl, const Sequence< PropertyValue >&)
    throw (RuntimeException)
{
    if(aUrl.Complete.equals(our_sTestAction))
        printf("WHEEEEEE Test Action invoked\n");
    else
        throw RuntimeException();
}

void WebdavuiDispatch::addStatusListener(const Reference< XStatusListener >& xListener, const URL& aUrl)
    throw (RuntimeException)
{
    OUString aLabel;

    if(aUrl.Complete.equals(our_sTestAction))
        aLabel = our_sTestActionLabel;

    FeatureStateEvent aEvent(
        static_cast<cppu::OWeakObject*>(this),
        aUrl,
        aLabel,
        true,
        false,
        makeAny(aLabel));

    xListener->statusChanged(aEvent);
}

void ::com::lanedo::webdavui::registerWebdavuiInterceptor(com::sun::star::uno::Reference<com::sun::star::frame::XDispatchProviderInterception> const& xInterception)
{
    xInterception->registerDispatchProviderInterceptor(Reference< XDispatchProviderInterceptor >(new WebdavuiInterceptor()));
}
