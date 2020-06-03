import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QuickFuture 1.0
import MesResult 1.0

import "../theme"
import "../components"
import "../helpers/login.js" as LoginLogic

Page {
    readonly property string terms:
        "By creating an account, you agree to Virgil's " +
        "<a style='text-decoration: none; color: %1' href='https://virgilsecurity.com/terms-of-service/'>Terms of Service</a> " +
        "and <a style='text-decoration: none; color: %1' href='https://virgilsecurity.com/privacy-policy/'>Privacy Policy</a>"

    background: Rectangle {
        color: Theme.mainBackgroundColor
    }

    header: Header {
        title: "Register"
        showBackButton: !form.isLoading
    }

    Form {

        id: form

        FormInput {
            id: username
            objectName: "fiRegisterUsername"
            label: "Username"
        }

        FormPrimaryButton {
            onClicked: signUp(username.text)
            objectName: "btnCreateAccount"
            text: "Create account"
        }

        FormLabel {
            text: terms.arg(Theme.buttonPrimaryColor)
        }
    }

    footer: Footer {}

    function signUp(user) {
        if (LoginLogic.validateUser(user)) {
            form.showLoading(qsTr("Registering %1...".arg(user)))

            var future = Messenger.signUp(user)

            Future.onFinished(future, function(result) {
                form.hideLoading()

                console.log("registration result: %1".arg(Future.result(future)))
                if (Future.result(future) === Result.MRES_OK) {
                    mainView.lastSignedInUser = user
                    mainView.showContacts("Contacts", null, true, true)

                    return
                }

                showPopupError(qsTr("Something went wrong :("))
            })
        } else {
            showPopupError(qsTr("Incorrect user name"))
        }
    }
}
