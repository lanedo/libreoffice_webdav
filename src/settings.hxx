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

#ifndef __WEBDAVUI_SETTINGS_HXX__
#define __WEBDAVUI_SETTINGS_HXX__

#include <com/sun/star/lang/XMultiComponentFactory.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/container/XNameContainer.hpp>

namespace css = com::sun::star;
using namespace css::uno;
using namespace css::lang;
using namespace css::container;
using rtl::OUString;

namespace WebDAVUI {

namespace LocalizedStrings {
    const OUString windowTitleOpen (RTL_CONSTASCII_USTRINGPARAM ("windowTitleOpen"));
    const OUString windowTitleSave (RTL_CONSTASCII_USTRINGPARAM ("windowTitleSave"));
    const OUString windowTitleConfig (RTL_CONSTASCII_USTRINGPARAM ("windowTitleConfig"));
    const OUString contentListFailure (RTL_CONSTASCII_USTRINGPARAM ("contentListFailure"));
    const OUString windowTitleError (RTL_CONSTASCII_USTRINGPARAM ("windowTitleError"));
    const OUString emptyFilename (RTL_CONSTASCII_USTRINGPARAM ("emptyFilename"));
    const OUString existingFilename (RTL_CONSTASCII_USTRINGPARAM ("existingFilename"));
}

namespace ImageKeys {
    const OUString imageURLLogo (RTL_CONSTASCII_USTRINGPARAM ("logoImage"));
}

class Settings
{
private:
    Reference< XComponentContext > mxContext;
    Reference< XMultiComponentFactory > mxMCF;
    Reference< XMultiServiceFactory > mxCfgProvider;
    Reference< XNameAccess > settingsAccess;
    Reference< XNameAccess > translationAccess;
    Reference< XNameAccess > imagesAccess;

    Reference< XNameAccess > createConfigurationView (const OUString &component);

    bool getStringValueByReference (Reference< css::container::XNameAccess >& xAccess,
                                    const OUString& aKeyName, OUString& aValue);

public:
    Settings (const Reference< XComponentContext > &rxContext);

    OUString getStringValue (const OUString& aKeyName);
    OUString getRemoteServerName ();

    bool setStringValue (const OUString& aKeyName, const OUString& aValue);
    bool setRemoteServerName (const OUString& aValue);

    Reference< XInterface > createDialog (const OUString& dialogName);
    OUString localizedString (const char *foo);
    OUString localizedString (OUString key);

    OUString getImageURL (const OUString& aKeyName);
};

}

#endif /* __WEBDAVUI_SETTINGS_HXX__ */
