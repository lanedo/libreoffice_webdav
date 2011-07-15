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
#include <com/sun/star/awt/XControlContainer.hpp>
#include <com/sun/star/awt/XDialogProvider2.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/deployment/PackageInformationProvider.hpp>
#include <com/sun/star/deployment/XPackageInformationProvider.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>

using rtl::OUString;
using namespace css::awt;
using namespace css::beans;
using namespace css::container;
using namespace css::deployment;
using namespace css::lang;
using namespace css::uno;
using css::lang::XMultiComponentFactory;
using css::awt::Key::RETURN;

namespace WebDAVUI {

/* Location of configuration data */
static const OUString settingsComponent (RTL_CONSTASCII_USTRINGPARAM ("com.lanedo.webdavui.ConfigurationData/Settings"));
static const OUString remoteServerKey (RTL_CONSTASCII_USTRINGPARAM ("webdavURL"));

static const OUString translationsComponent (RTL_CONSTASCII_USTRINGPARAM ("com.lanedo.webdavui.ConfigurationData/Translations"));


Settings::Settings (const Reference< XComponentContext > &rxContext) : mxContext (rxContext)
{
    printf ("Settings::Settings\n");
    mxMCF = mxContext->getServiceManager ();
    Reference< XMultiServiceFactory > multiServiceFactory (
        mxContext->getServiceManager(), UNO_QUERY_THROW);
    loadSettings (multiServiceFactory);
    loadTranslations ();
}


Reference< XInterface > Settings::createDialog (const OUString& dialogName)
{
    Reference< XPackageInformationProvider> infoProvider =
        PackageInformationProvider::get (mxContext);
    OUString dialogFile (OUString::createFromAscii ("/") + dialogName + OUString::createFromAscii (".xdl"));
    OUString packageUrl (infoProvider->getPackageLocation (
                         OUString::createFromAscii ("com.lanedo.webdavui")));
    if (packageUrl.getLength () == 0)
        packageUrl = OUString::createFromAscii (
            "file:///usr/lib/libreoffice/share/extensions/webdavui");
    OUString dialogUrl (packageUrl + dialogFile);
    printf ("Settings::createDialog: Loading UI from %s...\n",
            OUStringToOString (dialogUrl, RTL_TEXTENCODING_UTF8).getStr ());

    Reference< XInterface > dialogProvider =
        mxMCF->createInstanceWithContext (
            OUString::createFromAscii("com.sun.star.awt.DialogProvider2"), mxContext);
    Reference< XDialogProvider2 > dialogProvider2 (dialogProvider, UNO_QUERY);
    return dialogProvider2->createDialog (dialogUrl);
}

bool Settings::getStringValueByReference (Reference<XNameAccess>& xAccess,
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
    getStringValueByReference (settingsAccess, aKeyName, aValue);
    return aValue;
}

OUString Settings::getRemoteServerName ()
{
    OUString remoteServerName (getStringValue (remoteServerKey));
    if (remoteServerName.getLength() == 0)
        remoteServerName = OUString::createFromAscii ("http://localhost/dav/");
    return remoteServerName;
}

bool Settings::loadSettings (Reference< XMultiServiceFactory > const & factory)
{
    printf ("Settings::loadSettings\n");
    OSL_ASSERT (factory.is());
    const OUString kConfigurationProviderService (
        RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.configuration.ConfigurationProvider"));
    const OUString kReadOnlyViewService (
        RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.configuration.ConfigurationAccess"));

    try
    {
        mxCfgProvider = Reference< XMultiServiceFactory > (
            factory->createInstance (kConfigurationProviderService), UNO_QUERY);
        OSL_ENSURE (xCfgProvider.is (), "Failed to create ConfigurationProvider");
        if (!mxCfgProvider.is())
            return false;

        css::beans::NamedValue aPath (OUString (RTL_CONSTASCII_USTRINGPARAM ("nodepath")),
                                      makeAny (settingsComponent) );
        Sequence< Any > aArgs (1);
        aArgs[0] <<=  aPath;

        settingsAccess = Reference< XNameAccess > (mxCfgProvider->createInstanceWithArguments (kReadOnlyViewService, aArgs), UNO_QUERY);
    }
    catch (Exception & e)
    {
        printf ("Failed to instantiate XNameAccess: %s\n",
                OUStringToOString (e.Message, RTL_TEXTENCODING_ASCII_US).getStr ());
        return false;
    }

    return true;
}

bool Settings::loadTranslations ()
{
    const OUString kConfigurationProviderService (
        RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.configuration.ConfigurationProvider"));
    const OUString kReadOnlyViewService (
        RTL_CONSTASCII_USTRINGPARAM ("com.sun.star.configuration.ConfigurationAccess"));

    try
    {
        mxCfgProvider = Reference< XMultiServiceFactory > (
            mxMCF->createInstanceWithContext (kConfigurationProviderService, mxContext), UNO_QUERY);
        OSL_ENSURE (xCfgProvider.is (), "Failed to create ConfigurationProvider");
        if (!mxCfgProvider.is())
            return false;

        css::beans::NamedValue aPath (OUString (RTL_CONSTASCII_USTRINGPARAM ("nodepath")),
                                      makeAny (translationsComponent) );
        Sequence< Any > aArgs (1);
        aArgs[0] <<=  aPath;

        translationAccess = Reference< XNameAccess > (mxCfgProvider->createInstanceWithArguments (kReadOnlyViewService, aArgs), UNO_QUERY);
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
        PropertyValue aProperty;
        aProperty.Name  = OUString (RTL_CONSTASCII_USTRINGPARAM ("nodepath"));
        aProperty.Value = makeAny (settingsComponent);
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
    return setStringValue (remoteServerKey, aValue);
}

OUString Settings::localizedString (OUString key)
{
    OUString aValue;
    getStringValueByReference (translationAccess, key, aValue);
    return aValue;
}

}

