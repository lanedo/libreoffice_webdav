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

#include "configdialog.hxx"
#include <osl/diagnose.h>
#include <rtl/ustring.hxx>
#include <cppuhelper/implbase1.hxx>
#include <com/sun/star/awt/Key.hpp>
#include <com/sun/star/awt/WindowAttribute.hpp>
#include <com/sun/star/awt/XButton.hpp>
#include <com/sun/star/awt/XControl.hpp>
#include <com/sun/star/awt/XControlContainer.hpp>
#include <com/sun/star/awt/XControlModel.hpp>
#include <com/sun/star/awt/XDialog.hpp>
#include <com/sun/star/awt/XDialogProvider2.hpp>
#include <com/sun/star/awt/XWindowPeer.hpp>
#include <com/sun/star/awt/XMessageBox.hpp>
#include <com/sun/star/awt/XItemList.hpp>
#include <com/sun/star/awt/XListBox.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/deployment/PackageInformationProvider.hpp>
#include <com/sun/star/deployment/XPackageInformationProvider.hpp>
#include <com/sun/star/frame/FrameSearchFlag.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/ucb/XSimpleFileAccess.hpp>

#include <cstdio> // TEMPORARY: for puts

using rtl::OUString;
using namespace css::awt;
using namespace css::beans;
using namespace css::container;
using namespace css::deployment;
using namespace css::frame;
using namespace css::lang;
using namespace css::uno;
using css::lang::XMultiComponentFactory;
using css::awt::Key::RETURN;

namespace WebDAVUI {

/* Action listener */
class ConfigDialogActionListener : public ::cppu::WeakImplHelper1< css::awt::XActionListener >
{
private:
    ConfigDialog * const owner;

public:
    ConfigDialogActionListener (ConfigDialog * const _owner)
       : owner (_owner) { }

    // XEventListener
    virtual void SAL_CALL disposing (const css::lang::EventObject &aEventObj) throw (css::uno::RuntimeException)
    {
        puts ("dispose action listener");
    }

    // XActionListener
    virtual void SAL_CALL actionPerformed (const css::awt::ActionEvent &rEvent) throw (css::uno::RuntimeException)
    {
        /* Obtain the name of the control the event originated from */
        Reference< XControl > control (rEvent.Source, UNO_QUERY);
        Reference< XControlModel > controlModel = control->getModel ();
        Reference< XPropertySet > controlProps (controlModel, UNO_QUERY);
        css::uno::Any aValue = controlProps->getPropertyValue (OUString::createFromAscii ("Name"));
        OUString controlName;
        aValue >>= controlName;
        printf ("ConfigDialog::actionPerformed %s\n",
                OUStringToOString (controlName, RTL_TEXTENCODING_UTF8).getStr ());

        if (controlName.equalsAscii ("SaveButton"))
        {
            owner->saveChanges ();
        }
        else if (controlName.equalsAscii ("CancelButton"))
        {
            owner->closeDialog ();
        }
    }
};

/* Key listener */
class ConfigDialogKeyListener : public ::cppu::WeakImplHelper1< css::awt::XKeyListener >
{
private:
    ConfigDialog * const owner;

public:
    ConfigDialogKeyListener (ConfigDialog * const _owner)
       : owner (_owner) { }

    // XEventListener
    virtual void SAL_CALL disposing (const css::lang::EventObject &aEventObj) throw (css::uno::RuntimeException)
    {
        puts ("dispose key listener");
    }

    // XKeyListener
    virtual void SAL_CALL keyPressed (const css::awt::KeyEvent &rEvent) throw (css::uno::RuntimeException)
    {
        /* Obtain the name of the control the event originated from */
        Reference< XControl > control (rEvent.Source, UNO_QUERY);
        Reference< XControlModel > controlModel = control->getModel ();
        Reference< XPropertySet > controlProps (controlModel, UNO_QUERY);
        css::uno::Any aValue = controlProps->getPropertyValue (OUString::createFromAscii ("Name"));
        OUString controlName;
        aValue >>= controlName;

        puts ("key pressed");
    }

    virtual void SAL_CALL keyReleased (const css::awt::KeyEvent &rEvent) throw (css::uno::RuntimeException)
    {
        puts ("key released");
    }
};


/* Dialog construction */

ConfigDialog::ConfigDialog( const Reference< css::uno::XComponentContext > &rxContext,
                            const Reference< css::frame::XFrame >          &rxFrame,
                            WebDAVUI::Settings*                             rSettings) : mxContext ( rxContext ),
                                                                                         mxFrame ( rxFrame ),
                                                                                         mSettings ( rSettings)
{
    puts ("dialog ctor");

    mxMCF = mxContext->getServiceManager ();

    // Create the toolkit to have access to it later
    mxToolkit = Reference< XToolkit >( mxMCF->createInstanceWithContext(
                                        OUString( RTL_CONSTASCII_USTRINGPARAM(
                                            "com.sun.star.awt.Toolkit" )), mxContext), UNO_QUERY );

    createDialog ();
}

void ConfigDialog::createDialog (void)
{
    dialog = mSettings->createDialog (OUString::createFromAscii ("ConfigDialog"));
    Reference< XDialog > realDialog (dialog, UNO_QUERY);

    realDialog->setTitle (mSettings->localizedString ("Configure the Cloud"));

    /* Put the dialog in a window */
    Reference< XControl > control(dialog, UNO_QUERY);
    Reference< XWindow > window(control, UNO_QUERY);
    window->setVisible(true);
    control->createPeer(mxToolkit,NULL);

    OUString remoteServer (mSettings->getRemoveServerName ());

    Reference< XControlContainer > controlContainer (dialog, UNO_QUERY);
    Reference< XControl > entryControl =
        controlContainer->getControl (OUString::createFromAscii ("LocationEntry"));
    locationEntryModel = entryControl->getModel ();
    Reference< XPropertySet > entryProps (locationEntryModel, UNO_QUERY);
    entryProps->setPropertyValue(OUString::createFromAscii("Text"), makeAny (remoteServer));

    /* Get the open/save button */
    Reference< XControl > saveButton =
        controlContainer->getControl (OUString::createFromAscii ("SaveButton"));
    Reference< XControlModel > saveButtonModel =
        saveButton->getModel ();

    Reference< XPropertySet > openProps (saveButtonModel, UNO_QUERY);


    /* Create event listeners */
    Reference< XActionListener > actionListener =
        static_cast< XActionListener *> (new ConfigDialogActionListener (this));

    Reference< XKeyListener > keyListener =
        static_cast< XKeyListener *> (new ConfigDialogKeyListener (this));

    Reference< XButton > saveButtonControl (saveButton, UNO_QUERY);
    saveButtonControl->addActionListener (actionListener);


    /* Connect cancel button to action listener */
    Reference< XInterface > cancelObject =
        controlContainer->getControl (OUString::createFromAscii ("CancelButton"));
    Reference< XButton > cancelControl (cancelObject, UNO_QUERY);

    cancelControl->addActionListener (actionListener);
}


void ConfigDialog::show (void)
{
    /* Execute the clear */
    Reference< XDialog > xDialog(dialog,UNO_QUERY);
    xDialog->execute();

    /* After execution: get the XComponent interface of the dialog
     * and dispose the dialog.
     */
    Reference< XComponent > xComponent(dialog,UNO_QUERY);
    xComponent->dispose();
}

void ConfigDialog::saveChanges (void)
{
    Reference< XPropertySet > entryProps (locationEntryModel, UNO_QUERY);
    Any aValue = entryProps->getPropertyValue (OUString::createFromAscii ("Text"));
    OUString remoteServer;
    aValue >>= remoteServer;
    mSettings->setRemoteServerName (remoteServer);
    closeDialog ();
}

void ConfigDialog::closeDialog (void)
{
    Reference< XDialog > xDialog(dialog,UNO_QUERY);
    xDialog->endExecute();
}

}

