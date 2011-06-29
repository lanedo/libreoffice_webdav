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


#include "settings.hxx"
#include <cstdio>
#include <osl/diagnose.h>
#include <rtl/ustring.hxx>
#include <cppuhelper/implbase1.hxx>
#include <com/sun/star/awt/Key.hpp>
#include <com/sun/star/awt/XDialogProvider2.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>

using rtl::OUString;
using namespace css::awt;
using namespace css::beans;
using namespace css::container;
using namespace css::lang;
using namespace css::uno;
using css::lang::XMultiComponentFactory;
using css::awt::Key::RETURN;

namespace WebDAVUI {

Settings::Settings (const Reference< css::uno::XComponentContext > &rxContext) : mxContext (rxContext)
{
    printf ("Settings::Settings\n");
    mxMCF = mxContext->getServiceManager ();
    Reference< css::lang::XMultiServiceFactory > multiServiceFactory (
        mxContext->getServiceManager(), UNO_QUERY_THROW);
    loadSettings (multiServiceFactory);
}

bool Settings::getStringValueByReference (Reference<css::container::XNameAccess>& xAccess,
    const OUString& aKeyName, OUString& aValue)
{
    OSL_ASSERT(xAccess.is());
    xAccess->getByName(aKeyName) >>= aValue;
    printf ("Settings::getStringValueByReference %s → '%s'\n",
        OUStringToOString( aKeyName, RTL_TEXTENCODING_ASCII_US ).getStr(),
        OUStringToOString( aValue, RTL_TEXTENCODING_ASCII_US ).getStr());
    return aValue.getLength() != 0;
}

OUString Settings::getStringValue (const OUString& aKeyName)
{
    OUString aValue;
    Reference<css::container::XNameAccess > xChildAccess(mxIface, UNO_QUERY_THROW);
    getStringValueByReference (xChildAccess, aKeyName, aValue);
    return aValue;
}

OUString Settings::getRemoveServerName ()
{
    OUString remoteServerName (getStringValue (OUString::createFromAscii ("ooInetHTTPProxyName")));
    if (remoteServerName.getLength() == 0)
        remoteServerName = OUString::createFromAscii ("http://localhost/dav/");
    return remoteServerName;
}

bool Settings::loadSettings (Reference< css::lang::XMultiServiceFactory > const & factory)
{
    printf ("Settings::loadSettings\n");
    OSL_ASSERT (factory.is());
    const OUString kConfigurationProviderService (
        RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.configuration.ConfigurationProvider"));
    const OUString kReadOnlyViewService (
        RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.configuration.ConfigurationAccess"));
    const OUString kComponent (RTL_CONSTASCII_USTRINGPARAM("org.openoffice.Inet/Settings"));
    const OUString kServerDefinition (RTL_CONSTASCII_USTRINGPARAM ("ooInetProxyType"));

    try
    {
        mxCfgProvider = Reference< css::lang::XMultiServiceFactory > (
            factory->createInstance (kConfigurationProviderService), UNO_QUERY);
        OSL_ENSURE (xCfgProvider.is (), "Failed to create ConfigurationProvider");
        if (!mxCfgProvider.is())
            return false;

        css::beans::NamedValue aPath (OUString (RTL_CONSTASCII_USTRINGPARAM ("nodepath")),
                                      makeAny (kComponent) );
        Sequence< Any > aArgs (1);
        aArgs[0] <<=  aPath;
        mxIface = mxCfgProvider->createInstanceWithArguments (kReadOnlyViewService, aArgs);
        Reference<css::container::XNameAccess > xAccess (mxIface, UNO_QUERY_THROW);
        xAccess->getByName (kServerDefinition) >>= mxIface;
    }
    catch (Exception & e)
    {
        printf ("Failed to instantiate XNameAccess: %s\n",
                OUStringToOString (e.Message, RTL_TEXTENCODING_ASCII_US).getStr ());
        return false;
    }

    return true;
}

bool Settings::setStringValue (const OUString& aKeyName, const OUString& aValue)
{
    printf ("Settings::setStringValue\n");
    if (getStringValue (aKeyName) == aValue)
        return false;

    try
    {
        OUString aConfigRoot (RTL_CONSTASCII_USTRINGPARAM ("org.openoffice.Inet/Settings"));
        PropertyValue aProperty;
        aProperty.Name  = OUString (RTL_CONSTASCII_USTRINGPARAM ("nodepath"));
        aProperty.Value = makeAny (aConfigRoot);
        Sequence< Any > aArgumentList (1);
        aArgumentList[0] = makeAny (aProperty);
        Reference< XInterface > m_xConfigurationUpdateAccess = mxCfgProvider->createInstanceWithArguments (OUString (
            RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.configuration.ConfigurationUpdateAccess")),
            aArgumentList);

        Reference< XPropertySet > xPropertySet (m_xConfigurationUpdateAccess, UNO_QUERY_THROW);
        xPropertySet->setPropertyValue (aKeyName, makeAny (aValue));
        Reference< css::util::XChangesBatch > xChangesBatch (m_xConfigurationUpdateAccess, UNO_QUERY_THROW);
        xChangesBatch->commitChanges ();
        return true;
    }
    catch (RuntimeException & e) {
        printf ("Failed to update string setting: %s\n",
                OUStringToOString (e.Message, RTL_TEXTENCODING_ASCII_US).getStr ());
    }

    return false;
}

bool Settings::setRemoteServerName (const OUString& aValue)
{
    return setStringValue (OUString::createFromAscii ("ooInetHTTPProxyName"), aValue);
}

}

