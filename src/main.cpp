// This file is part of Jelly Bean. Strangetest. It is subject to the license
// terms in the LICENSE.txt file found in the top-level directory of this
// distribution. No part of this project, including this file, may be copied,
// modified, propagated, or distributed except according to the terms contained
// in the LICENSE.txt file.

#include <stdio.h>

#include <Qt>
#include <QApplication>
#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QKeySequence>
#include <QMainWindow>
#include <QMenuBar>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTreeWidget>


namespace {


enum MenuType
{
    MENU_FILE,
    MENU_ACCOUNT,
    MENU_TRANSACTION,
    MENU_REPORT,

    MENU_COUNT,
};


enum ActionType
{
    ACTION_NEW_FILE,
    ACTION_QUIT,

    ACTION_NEW_ACCOUNT,
    ACTION_EDIT_ACCOUNT,
    ACTION_DELETE_ACCOUNT,
    ACTION_VIEW_ACCOUNT,

    ACTION_NEW_TRANSACTION,
    ACTION_EDIT_TRANSACTION,
    ACTION_DELETE_TRANSACTION,
    ACTION_VIEW_TRANSACTION,

    ACTION_BALANCE_REPORT,
    ACTION_INCOME_REPORT,
    ACTION_CASH_REPORT,

    ACTION_COUNT,
};


struct UI
{
    QMainWindow *window;
    QMenu *menus[MENU_COUNT];
    QAction *actions[ACTION_COUNT];

    QTabWidget *ledger;
    QTreeWidget *accounts;
};


class TransactionEntryItemDelegate final : public QStyledItemDelegate
{
public:
    TransactionEntryItemDelegate(
        QAbstractItemModel *accounts,
        QObject *parent = nullptr);

    QWidget *createEditor(
        QWidget *parent,
        const QStyleOptionViewItem &option,
        const QModelIndex &index) const override;

    //void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(
        QWidget *editor,
        const QStyleOptionViewItem &option,
        const QModelIndex &index) const override;

private:
    QAbstractItemModel *m_accounts;
};


TransactionEntryItemDelegate::TransactionEntryItemDelegate(
    QAbstractItemModel *accounts, QObject *parent)
    : QStyledItemDelegate(parent), m_accounts(accounts)
{
}


QWidget *
TransactionEntryItemDelegate::createEditor(
    QWidget *parent,
    const QStyleOptionViewItem & /* option */,
    const QModelIndex & /* index */) const
{
    QComboBox *editor = new QComboBox(parent);
    editor->setModel(m_accounts);

    return editor;
}


void
TransactionEntryItemDelegate::setModelData(
    QWidget *widget,
    QAbstractItemModel *model,
    const QModelIndex &index) const
{
    QComboBox *editor = qobject_cast<QComboBox *>(widget);
    QString value = editor->currentText();
    model->setData(index, value);
}


void
TransactionEntryItemDelegate::updateEditorGeometry(
    QWidget *editor,
    const QStyleOptionViewItem &option,
    const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}


void
close_file(UI *ui)
{
    for (unsigned i = 0; i < ACTION_COUNT; ++i)
    {
        if ((i != ACTION_NEW_FILE) && (i != ACTION_QUIT))
        {
            ui->actions[i]->setEnabled(false);
        }
    }

    for (unsigned i = 0; i < MENU_COUNT; ++i)
    {
        if (i != MENU_FILE)
        {
            QMenu *menu = ui->menus[i];
            menu->setEnabled(false);
        }
    }

    QTabWidget *ledger = ui->ledger;
    for (int i = ledger->count(); i; --i)
    {
        QWidget *tab = ledger->widget(i - 1);
        delete tab;
    }

    ui->accounts = nullptr;
}


void
new_file(UI *ui)
{
    close_file(ui);

    QTreeWidget *accounts = new QTreeWidget();
    accounts->setAlternatingRowColors(true);
    accounts->setColumnCount(1);
    accounts->setHeaderLabel("Account");
    ui->accounts = accounts;

    QTabWidget *ledger = ui->ledger;
    ledger->addTab(accounts, "Accounts");

    ui->menus[MENU_ACCOUNT]->setEnabled(true);
    ui->actions[ACTION_NEW_ACCOUNT]->setEnabled(true);
}


void
create_account(UI *ui)
{
    bool ok;
    QString text = QInputDialog::getText(
        ui->window, "Create Account",
        "Account name", QLineEdit::Normal,
        QString(), &ok);
    if (ok)
    {
        QTreeWidgetItem *account = new QTreeWidgetItem(ui->accounts);
        account->setText(0, text);

        bool enable_transactions = ui->accounts->topLevelItemCount();
        ui->menus[MENU_TRANSACTION]->setEnabled(enable_transactions);
        ui->actions[ACTION_NEW_TRANSACTION]->setEnabled(enable_transactions);
    }
}


void
create_transaction(UI *ui)
{
    QDialog dialog(ui->window);
    dialog.setWindowTitle("Create Transaction");

    QFormLayout *layout = new QFormLayout;
    dialog.setLayout(layout);

    QDateEdit *date = new QDateEdit(QDate::currentDate());
    date->setCalendarPopup(true);
    layout->addRow("Date:", date);

    QLineEdit *party = new QLineEdit;
    layout->addRow("Payee:", party);

    QLineEdit *note = new QLineEdit;
    layout->addRow("Note:", note);

    QTableWidget *entries = new QTableWidget(10, 3);
    TransactionEntryItemDelegate *delegate = new TransactionEntryItemDelegate(
            ui->accounts->model(), entries);
    entries->setItemDelegateForColumn(0, delegate);
    entries->setHorizontalHeaderLabels({"Account", "Debit", "Credit"});
    entries->verticalHeader()->setVisible(false);
    QHeaderView *header = entries->horizontalHeader();
    header->setSectionsClickable(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    layout->addRow("Entries:", entries);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);

    int result = dialog.exec();
    if (result == QDialog::Accepted)
    {
        qDebug() << "Date is:" << date->date();
        qDebug() << "Party name is:" << party->text();
        qDebug() << "Note is:" << party->text();

        for (int row = 0; row < entries->rowCount(); ++row)
        {
            for (int col = 0; col < entries->columnCount(); ++col)
            {
                QTableWidgetItem *item = entries->item(row, col);
                if (item)
                {
                    qDebug() << row << "," << col << ":" << item->data(Qt::EditRole);
                }
                else
                {
                    qDebug() << row << "," << col << ": No item!";
                }
            }
        }
    }
}


} // unnamed namespace


int
main(int argc, char **argv)
{
    UI ui = {};

    QApplication app(argc, argv);
    app.setApplicationName("jellybean");
    app.setApplicationDisplayName("Jelly Bean");
    app.setApplicationVersion("0.1.0");

    QMainWindow window;
    window.setWindowTitle("Jelly Bean");
    ui.window = &window;

    QTabWidget *ledger = new QTabWidget;
    window.setCentralWidget(ledger);
    ui.ledger = ledger;

    // TODO: Make a "parent-less" menu bar for Mac platforms?
    QMenuBar *menubar = window.menuBar();

    //
    // File menu
    //
    QMenu *menu = menubar->addMenu("&File");
    ui.menus[MENU_FILE] = menu;

    QAction *action = menu->addAction("&New File", QKeySequence::New, [&ui]() { new_file(&ui); });
    ui.actions[ACTION_NEW_FILE] = action;

    menu->addSeparator();
    action = menu->addAction("&Quit", QKeySequence::Quit, &app, &QApplication::quit, Qt::QueuedConnection);
    ui.actions[ACTION_QUIT] = action;

    //
    // Account menu
    //
    menu = menubar->addMenu("&Account");
    menu->setEnabled(false);
    ui.menus[MENU_ACCOUNT] = menu;

    action = menu->addAction("&New Account", QKeySequence("Shift+A"), [&ui]() { create_account(&ui); });
    action->setEnabled(false);
    ui.actions[ACTION_NEW_ACCOUNT] = action;

    action = menu->addAction("&Edit Account");
    action->setEnabled(false);
    ui.actions[ACTION_EDIT_ACCOUNT] = action;

    action = menu->addAction("&Delete Account");
    action->setEnabled(false);
    ui.actions[ACTION_DELETE_ACCOUNT] = action;

    action = menu->addAction("&View Account");
    action->setEnabled(false);
    ui.actions[ACTION_VIEW_ACCOUNT] = action;

    //
    // Transaction menu
    //
    menu = menubar->addMenu("&Transaction");
    menu->setEnabled(false);
    ui.menus[MENU_TRANSACTION] = menu;

    action = menu->addAction("&New Transaction", QKeySequence("Shift+T"), [&ui]() { create_transaction(&ui); });
    action->setEnabled(false);
    ui.actions[ACTION_NEW_TRANSACTION] = action;

    action = menu->addAction("&Edit Transaction");
    action->setEnabled(false);
    ui.actions[ACTION_EDIT_TRANSACTION] = action;

    action = menu->addAction("&Delete Transaction");
    action->setEnabled(false);
    ui.actions[ACTION_DELETE_TRANSACTION] = action;

    action = menu->addAction("&View Transaction");
    action->setEnabled(false);
    ui.actions[ACTION_VIEW_TRANSACTION] = action;

    //
    // Report menu
    //
    menu = menubar->addMenu("&Report");
    menu->setEnabled(false);
    ui.menus[MENU_REPORT] = menu;

    action = menu->addAction("Balance Sheet");
    action->setEnabled(false);
    ui.actions[ACTION_BALANCE_REPORT] = action;

    action = menu->addAction("Income Report");
    action->setEnabled(false);
    ui.actions[ACTION_INCOME_REPORT] = action;

    action = menu->addAction("Cash Flow");
    action->setEnabled(false);
    ui.actions[ACTION_CASH_REPORT] = action;


    window.show();
    int result = app.exec();
    return result;
}
