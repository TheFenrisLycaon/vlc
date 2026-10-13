/*****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#include "heimdall_stub_modules.hpp"

#include <QTest>
#include <QSignalSpy>
#include <QThreadPool>
#include "dialogs/dialogs/dialogmodel.hpp"
#include <heimdall_cxx_helpers.hpp>

#include "../util/renderer_manager.hpp"

class TestHeimdallDialogModel : public QObject
{
    Q_OBJECT

    enum UserAnswer {
        Dismiss,
        PostLogin,
        PostAction1,
        PostAction2
    };

private slots:
    void initTestCase() {
        m_env = std::make_unique<HeimdallTestingEnv>();
        QVERIFY(m_env->init());
        m_intf = m_env->intf;
    }

    void cleanupTestCase() {
        m_intf = nullptr;
        m_env.reset();
    }

    void init() {
        HeimdallDialogModel::getInstance(m_intf);
        m_dialog = new HeimdallDialog();
        m_loginSpy = std::make_unique<QSignalSpy>(m_dialog, &HeimdallDialog::login);
        m_questionSpy = std::make_unique<QSignalSpy>(m_dialog, &HeimdallDialog::question);
        m_progressSpy = std::make_unique<QSignalSpy>(m_dialog, &HeimdallDialog::progress);
        m_progressUpdatedSpy = std::make_unique<QSignalSpy>(m_dialog, &HeimdallDialog::progressUpdated);
        m_canceledSpy = std::make_unique<QSignalSpy>(m_dialog, &HeimdallDialog::cancelled);
    }

    void cleanup() {
        m_loginSpy.reset();
        m_questionSpy.reset();
        m_progressSpy.reset();
        m_progressUpdatedSpy.reset();
        m_canceledSpy.reset();
        HeimdallDialogModel* model = HeimdallDialogModel::getInstance<false>();
        if (model) {
            model->setProvider(nullptr);
            HeimdallDialogModel::killInstance();
        }
        delete m_dialog;
    }

    void testLogin_data() {
        QTest::addColumn<UserAnswer>("userReply");
        QTest::addColumn<int>("expectedReply");

        QTest::newRow("login") << PostLogin << 1;
        QTest::newRow("dismiss") << Dismiss << 0;
        QTest::newRow("wrongAnswer") << PostAction1 << HEIMDALL_EGENERIC;
    }

    //failed state when no renderer manager is found
    void testLogin() {
        QFETCH(UserAnswer, userReply);
        QFETCH(int, expectedReply);

        QObject::connect(m_dialog, &HeimdallDialog::login, this, [&](
            DialogId dialogId, const QString & title,
            const QString & text, const QString & defaultUsername,
            bool){

            QCOMPARE(defaultUsername, "default username");
            QCOMPARE(title, "login title");
            QCOMPARE(text, "login message");

            switch(userReply)
            {
            case Dismiss:
                m_dialog->dismiss(dialogId);
                break;
            case PostLogin:
                m_dialog->post_login(dialogId, "username", "hunter2", false);
                break;
            case PostAction1:
            default:
                //bad behavior
                m_dialog->post_action1(dialogId);
                break;

            }
        });

        HeimdallDialogModel::getInstance<false>()->setProvider(m_dialog);

        std::atomic<bool> replied = false;
        int reply = -1;
        QString repliedUsername;
        QString repliedPassword;
        bool repliedStore = true;
        QThreadPool::globalInstance()->start([&](){
            char* username = nullptr;
            char* password = nullptr;
            reply = heimdall_dialog_wait_login(m_intf, &username, &password, &repliedStore,
                                  "default username", "login title",
                                  "login message");
            if (reply == 1) {
                repliedUsername = username;
                repliedPassword = password;
                free(username);
                free(password);
            }

            replied = true;
        });

        QVERIFY(QTest::qWaitFor([&replied](){ return replied == true; }));
        if (expectedReply == HEIMDALL_EGENERIC)
            QCOMPARE_LT(reply, 0);
        else
            QCOMPARE(reply, expectedReply);

        if (reply == 1) {
            QCOMPARE(repliedUsername, "username");
            QCOMPARE(repliedPassword, "hunter2");
            QCOMPARE(repliedStore, false);
        }

        QCOMPARE(m_questionSpy->count(), 0);
        QCOMPARE(m_progressSpy->count(), 0);
        QCOMPARE(m_progressUpdatedSpy->count(), 0);
        QCOMPARE(m_canceledSpy->count(), 0);
    }

    void testQuestion_data() {
        QTest::addColumn<UserAnswer>("userReply");
        QTest::addColumn<int>("expectedReply");

        QTest::newRow("action1") << PostAction1 << 1;
        QTest::newRow("action2") << PostAction2 << 2;
        QTest::newRow("dismiss") << Dismiss << 0;
        QTest::newRow("wrongAnswer") << PostLogin << HEIMDALL_EGENERIC;
    }

    //failed state when no renderer manager is found
    void testQuestion() {
        QFETCH(UserAnswer, userReply);
        QFETCH(int, expectedReply);

        QObject::connect(m_dialog, &HeimdallDialog::question, this,
                         [&](DialogId dialogId, const QString& title,
                            const QString& text, HeimdallDialog::QuestionType type,
                            const QString& cancel, const QString& action1, const QString& action2){

            QCOMPARE(action1, "action1 txt");
            QCOMPARE(action2, "action2 txt");
            QCOMPARE(cancel, "cancel txt");
            QCOMPARE(title, "title");
            QCOMPARE(text, "message");
            QCOMPARE(type, HeimdallDialog::QuestionType::QUESTION_WARNING);

            switch(userReply)
            {
            case PostAction1:
                m_dialog->post_action1(dialogId);
                break;
            case PostAction2:
                m_dialog->post_action2(dialogId);
                break;
            case Dismiss:
                m_dialog->dismiss(dialogId);
                break;
            case PostLogin:
                m_dialog->post_login(dialogId, "not", "expected"); //user answer the type
                break;
            }
        });

        HeimdallDialogModel::getInstance<false>()->setProvider(m_dialog);

        std::atomic<bool> replied = false;
        int reply = -1;
        QThreadPool::globalInstance()->start([&](){
            reply = heimdall_dialog_wait_question(m_intf, HEIMDALL_DIALOG_QUESTION_WARNING,
                                             "cancel txt", "action1 txt",
                                             "action2 txt", "title", "message");
            replied = true;
        });
        QVERIFY(QTest::qWaitFor([&replied](){ return replied == true; }));
        if (expectedReply == HEIMDALL_EGENERIC)
            QCOMPARE_LT(reply, 0);
        else
            QCOMPARE(reply, expectedReply);

        QCOMPARE(m_loginSpy->count(), 0);
        QCOMPARE(m_progressSpy->count(), 0);
        QCOMPARE(m_progressUpdatedSpy->count(), 0);
        QCOMPARE(m_canceledSpy->count(), 0);
    }

    //failed state when no renderer manager is found
    void testProgress() {
        QObject::connect(m_dialog, &HeimdallDialog::progress, this,
                         [&](DialogId, const QString& title, const QString& text,
                                bool b_indeterminate, float f_position, const QString& cancel){

            //QCOMPARE performs is fuzzy for floats
            QCOMPARE(b_indeterminate, false);
            QCOMPARE(f_position, 0.f);
            QCOMPARE(cancel, "cancel txt");
            QCOMPARE(title, "title");
            QCOMPARE(text, "message");
        });

        QObject::connect(m_dialog, &HeimdallDialog::progressUpdated, this,
                         [&](DialogId, float f_position, const QString& text){

            //QCOMPARE performs is fuzzy for floats
            QCOMPARE(f_position, 0.3f);
            QCOMPARE(text, "updated text");
        });

        HeimdallDialogModel::getInstance<false>()->setProvider(m_dialog);

        std::atomic<bool> replied = false;
        heimdall_dialog_id* dialogId;
        QThreadPool::globalInstance()->start([&](){
            dialogId = heimdall_dialog_display_progress(m_intf,
                                                   false, 0.f,
                                                   "cancel txt", "title", "message");
            replied = true;
        });
        QVERIFY(QTest::qWaitFor([&replied](){ return replied == true; }));
        QVERIFY(dialogId != nullptr);

        replied = false;
        int reply;
        QThreadPool::globalInstance()->start([&](){
            reply = heimdall_dialog_update_progress_text(m_intf, dialogId, 0.3f, "updated text");
            replied = true;
        });
        QVERIFY(QTest::qWaitFor([&replied](){ return replied == true; }));
        QCOMPARE(reply, HEIMDALL_SUCCESS);
        QCOMPARE(m_canceledSpy->count(), 0);

        replied = false;
        QThreadPool::globalInstance()->start([&](){
            heimdall_dialog_release(m_intf, dialogId);
            replied = true;
        });
        QVERIFY(QTest::qWaitFor([&replied](){ return replied == true; }));
        QCOMPARE(m_canceledSpy->count(), 1);

        QCOMPARE(m_loginSpy->count(), 0);
        QCOMPARE(m_questionSpy->count(), 0);
    }

    void testLateBinding() {
        QObject::connect(m_dialog, &HeimdallDialog::question, this,
                         [&](DialogId dialogId, const QString& ,
                             const QString& , HeimdallDialog::QuestionType ,
                             const QString& , const QString& , const QString& ){
            m_dialog->post_action1(dialogId);
        });

        std::atomic<bool> replied = false;
        int reply = -1;
        QThreadPool::globalInstance()->start([&](){
            reply = heimdall_dialog_wait_question(m_intf, HEIMDALL_DIALOG_QUESTION_WARNING,
                                             "cancel txt", "action1 txt",
                                             "action2 txt", "title", "message");
            replied = true;
        });
        //wait for the question to be asked
        //the dialog should block until HeimdallDialog is provided
        QTest::qWait(20);

        //set the dialog afterwards
        HeimdallDialogModel::getInstance<false>()->setProvider(m_dialog);

        QVERIFY(QTest::qWaitFor([&replied](){ return replied == true; }));
        //user replied with Action1
        QCOMPARE(reply, 1);

        QCOMPARE(m_loginSpy->count(), 0);
        QCOMPARE(m_progressSpy->count(), 0);
        QCOMPARE(m_progressUpdatedSpy->count(), 0);
        QCOMPARE(m_canceledSpy->count(), 0);
    }

    void testModelDestruction() {
        std::atomic<bool> replied = false;
        int reply = -1;
        QThreadPool::globalInstance()->start([&](){
            reply = heimdall_dialog_wait_question(m_intf, HEIMDALL_DIALOG_QUESTION_WARNING,
                                             "cancel txt", "action1 txt",
                                             "action2 txt", "title", "message");
            replied = true;
        });
        //wait for the question to be asked
        //the dialog should block until HeimdallDialog is provided
        QTest::qWait(20);

        HeimdallDialogModel::killInstance();

        QVERIFY(QTest::qWaitFor([&replied](){ return replied == true; }));

        QCOMPARE(reply, 0);
    }

    void testSameThreadWithoutDialog()
    {
        int reply = heimdall_dialog_wait_question(m_intf, HEIMDALL_DIALOG_QUESTION_WARNING,
                                             "cancel txt", "action1 txt",
                                             "action2 txt", "title", "message");
        QCOMPARE(reply, 0);
    }

    void testSameThreadWithDialog()
    {
        QObject::connect(m_dialog, &HeimdallDialog::question, this,
                         [&](DialogId dialogId, const QString& ,
                             const QString& , HeimdallDialog::QuestionType ,
                             const QString& , const QString& , const QString& ){
            m_dialog->post_action1(dialogId);
        });
        HeimdallDialogModel::getInstance<false>()->setProvider(m_dialog);

        int reply = heimdall_dialog_wait_question(m_intf, HEIMDALL_DIALOG_QUESTION_WARNING,
                                             "cancel txt", "action1 txt",
                                             "action2 txt", "title", "message");
        QCOMPARE(reply, 1);
    }

private:
    std::unique_ptr<HeimdallTestingEnv> m_env;
    qt_intf_t* m_intf = nullptr;

    HeimdallDialog* m_dialog = nullptr;

    std::unique_ptr<QSignalSpy> m_loginSpy;
    std::unique_ptr<QSignalSpy> m_questionSpy;
    std::unique_ptr<QSignalSpy> m_progressSpy;
    std::unique_ptr<QSignalSpy> m_progressUpdatedSpy;
    std::unique_ptr<QSignalSpy> m_canceledSpy;
};

QTEST_GUILESS_MAIN(TestHeimdallDialogModel)
#include "test_heimdall_dialog_model.moc"
