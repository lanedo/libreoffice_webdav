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

#include <stdio.h>

#include <osl/mutex.hxx>
#include <osl/thread.h>
#include <cppuhelper/factory.hxx>
#include <rtl/ustring.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/types.h>

#ifndef _COM_SUN_STAR_LANG_XSINGLESERVICEFACTORY_HPP_
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#endif

#include "webdavuiinterceptor.hxx"

namespace css = ::com::sun::star;

static void writeInfo(const css::uno::Reference< css::registry::XRegistryKey >& xRegistryKey,
                      const char*                                               pImplementationName,
                      const char*                                               pServiceName)
{
    ::rtl::OUStringBuffer sKey(256);
    sKey.append     (::rtl::OUString::createFromAscii(pImplementationName));
    sKey.appendAscii("/UNO/SERVICES/");
    sKey.append     (::rtl::OUString::createFromAscii(pServiceName));

    xRegistryKey->createKey(sKey.makeStringAndClear());
}

extern "C"
{
SAL_DLLPUBLIC_EXPORT void SAL_CALL component_getImplementationEnvironment(const sal_Char**  ppEnvTypeName,
                                                                          uno_Environment** /*ppEnv*/)
{
    *ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}


SAL_DLLPUBLIC_EXPORT sal_Bool SAL_CALL component_writeInfo(void* pServiceManager,
                                                           void* pRegistryKey)
{
    if (!pRegistryKey)
        return sal_False;

    try
    {
        css::uno::Reference< css::registry::XRegistryKey > xKey(reinterpret_cast< css::registry::XRegistryKey* >(pRegistryKey), css::uno::UNO_QUERY);

#if 0
        writeInfo( xKey, DESKTOPJOB_IMPLEMENTATION_NAME, DESKTOPJOB_SERVICE_NAME);
        writeInfo( xKey, FRAMEJOB_IMPLEMENTATION_NAME, FRAMEJOB_SERVICE_NAME);
#endif

        return sal_True;
    }
    catch(const css::registry::InvalidRegistryException&)
        { OSL_ENSURE( sal_False, "### InvalidRegistryException!" ); }

    return sal_False;
}

SAL_DLLPUBLIC_EXPORT void* SAL_CALL component_getFactory(const sal_Char* pImplName,
                                                         void*     pServiceManager,
                                                         void*     /*pRegistryKey*/)
{
    if ( !pServiceManager || !pImplName )
        return 0;

    css::uno::Reference< css::lang::XSingleServiceFactory > xFactory  ;
    css::uno::Reference< css::lang::XMultiServiceFactory >  xSMGR     (reinterpret_cast< css::lang::XMultiServiceFactory* >(pServiceManager), css::uno::UNO_QUERY);
    ::rtl::OUString                                         sImplName = ::rtl::OUString::createFromAscii(pImplName);

#if 0
    if (sImplName.equalsAscii(DESKTOPJOB_IMPLEMENTATION_NAME))
    {
        css::uno::Sequence< ::rtl::OUString > lNames(1);
        lNames[0] = ::rtl::OUString::createFromAscii(DESKTOPJOB_IMPLEMENTATION_NAME);
        xFactory = ::cppu::createSingleFactory(xSMGR, sImplName,  DesktopJob_createInstance, lNames);
    }
    if (sImplName.equalsAscii(FRAMEJOB_IMPLEMENTATION_NAME))
    {
        css::uno::Sequence< ::rtl::OUString > lNames(1);
        lNames[0] = ::rtl::OUString::createFromAscii(FRAMEJOB_IMPLEMENTATION_NAME);
        xFactory = ::cppu::createSingleFactory(xSMGR, sImplName,  FrameJob_createInstance, lNames);
    }
#endif


    if (!xFactory.is())
        return 0;

    xFactory->acquire();
    return xFactory.get();
}

} // extern C
