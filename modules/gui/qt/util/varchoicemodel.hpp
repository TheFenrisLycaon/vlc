/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef HEIMDALL_VAR_CHOICE_MODEL_HPP
#define HEIMDALL_VAR_CHOICE_MODEL_HPP

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "qt.hpp"
#include <QAbstractListModel>
#include <heimdall_cxx_helpers.hpp>
#include "varcommon_p.hpp"

extern "C" {
int HeimdallVarChoiceModel_on_variable_callback( heimdall_object_t * object, char const * , heimdall_value_t oldvalue, heimdall_value_t newvalue, void * data);
int HeimdallVarChoiceModel_on_variable_list_callback( heimdall_object_t * object, char const * , int action, heimdall_value_t* value, void * data);
}

/**
 * @brief The HeimdallVarChoiceModel class construct an Abstract List Model from a
 * heimdall_var with the HEIMDALL_VAR_HASCHOICE flag and a type amongst string, int, float, bool
 *
 * available roles are DisplayRole and CheckStateRole
 */
class HeimdallVarChoiceModel : public QAbstractListModel
{
    Q_OBJECT
public:
    Q_PROPERTY(bool hasCurrent READ hasCurrent NOTIFY hasCurrentChanged FINAL)

    template<typename T>
    HeimdallVarChoiceModel(T *p_object, const char* varName, QObject *parent = nullptr);

    ~HeimdallVarChoiceModel();

    //QAbstractListModel overridden functions
    Qt::ItemFlags flags(const QModelIndex &) const  override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    /**
     * @brief resetObject change the observed object.
     * @param object the new object to observe (can be null), the incoming object will be hold.
     * @return true if the object has the observed variable and if the variable has the right type.
     */
    template<typename T>
    bool resetObject(T *object);

    bool hasCurrent() const;
    int currentRow() const;

    static int on_variable_callback( heimdall_object_t * object, char const * , heimdall_value_t oldvalue, heimdall_value_t newvalue, void * data);
    static int on_variable_list_callback( heimdall_object_t * object, char const * , int action, heimdall_value_t* value, void * data);

public slots:
    Q_INVOKABLE void toggleIndex(int index);

private slots:
    void onDataUpdated(const heimdall_object_t* object, QVariant oldvalue, QVariant newvalue);
    void onListUpdated(const heimdall_object_t* object, int action, QVariant newvalue);

signals:
    void dataUpdated(const heimdall_object_t* object, QVariant oldvalue, QVariant newvalue, QPrivateSignal);
    void listUpdated(const heimdall_object_t* object, int action, QVariant newvalue, QPrivateSignal);
    void hasCurrentChanged( bool );

private:
    int updateData(const heimdall_object_t* object, const heimdall_value_t& oldvalue, const heimdall_value_t& newvalue);
    int updateList(const heimdall_object_t* object, int action, const heimdall_value_t* p_newvalue);

    QString heimdallValToString(const heimdall_value_t& value);
    QVariant heimdallValToVariant(const heimdall_value_t& a);

    //reference to the observed object. Can only be modified from the UI thread
    std::unique_ptr<HeimdallObjectHolder> m_object;
    int m_type;
    QString m_varname;

    QList< QVariant > m_values;
    QStringList m_titles;
    int m_current = -1;

    friend int HeimdallVarChoiceModel_on_variable_callback( heimdall_object_t * object, char const * , heimdall_value_t oldvalue, heimdall_value_t newvalue, void * data);
    friend int HeimdallVarChoiceModel_on_variable_list_callback( heimdall_object_t * object, char const * , int action, heimdall_value_t* value, void * data);
};


template<typename T>
HeimdallVarChoiceModel::HeimdallVarChoiceModel(T *p_object, const char* varName, QObject *parent)
    : QAbstractListModel(parent)
    , m_object(new HeimdallObjectHolderImpl<T>(nullptr))
    , m_varname(qfu(varName))
{
    connect(this, &HeimdallVarChoiceModel::dataUpdated, this, &HeimdallVarChoiceModel::onDataUpdated);
    connect(this, &HeimdallVarChoiceModel::listUpdated, this, &HeimdallVarChoiceModel::onListUpdated);
    resetObject(p_object);
}

template<typename T>
bool HeimdallVarChoiceModel::resetObject(T* p_object)
{
    beginResetModel();

    //clear the old model
    if (m_object->get())
    {
        var_DelCallback(m_object->get(), qtu(m_varname), HeimdallVarChoiceModel::on_variable_callback, this);
        var_DelListCallback(m_object->get(), qtu(m_varname), HeimdallVarChoiceModel::on_variable_list_callback, this);
        var_Destroy(m_object->get(), qtu(m_varname));
    }
    m_object->reset(p_object, true);

    m_values.clear();
    m_titles.clear();

    if (!m_object->get())
    {
        endResetModel();
        return true;
    }

    m_type = var_Type( m_object->get(), qtu(m_varname) );
    //we only handle variables with choice here
    if ( ! (m_type & HEIMDALL_VAR_HASCHOICE) )
    {
        m_object->reset((T*)nullptr, false);
        endResetModel();
        return false;
    }


    switch( m_type & HEIMDALL_VAR_CLASS )
    {
        case HEIMDALL_VAR_BOOL:
        case HEIMDALL_VAR_STRING:
        case HEIMDALL_VAR_INTEGER:
        case HEIMDALL_VAR_FLOAT:
            break;
        default:
            /* Variable doesn't exist or isn't handled */
            endResetModel();
            return false;
    }

    size_t count = 0;
    heimdall_value_t* val_list = nullptr;
    char** val_title = nullptr;
    if( var_Change( m_object->get(), qtu(m_varname), HEIMDALL_VAR_GETCHOICES, &count, &val_list, &val_title ) < 0 )
    {
        endResetModel();
        return false;
    }
    heimdall_value_t currentValue;
    if (var_Get( m_object->get(), qtu(m_varname), &currentValue) != HEIMDALL_SUCCESS)
    {
        endResetModel();
        return false;
    }
    QVariant currentValueVar = heimdallValToVariant(currentValue);
    if ( (m_type & HEIMDALL_VAR_CLASS) == HEIMDALL_VAR_STRING )
        free( currentValue.psz_string );

    int newCurrent = -1;
    for (size_t i = 0; i < count; i++)
    {
        QVariant var = heimdallValToVariant( val_list[i] );
        if (currentValueVar == var)
            newCurrent = i;
        m_values.append( var );
        if ( val_title[i] )
        {
            //display the title if present
            m_titles.append( qfu(val_title[i]) );
            free(val_title[i]);
        }
        else
        {
            //print the value otherwise
            m_titles.append( heimdallValToString(val_list[i]) );
        }
        if ( (m_type & HEIMDALL_VAR_CLASS) == HEIMDALL_VAR_STRING )
            free( val_list[i].psz_string );
    }
    if (m_current != newCurrent)
    {
        m_current = newCurrent;
        emit hasCurrentChanged(newCurrent != -1);
    }

    free(val_list);
    free(val_title);

    var_Create(m_object->get(), qtu(m_varname), m_type);
    var_AddCallback(m_object->get(), qtu(m_varname), HeimdallVarChoiceModel::on_variable_callback, this);
    var_AddListCallback(m_object->get(), qtu(m_varname), HeimdallVarChoiceModel::on_variable_list_callback, this);

    endResetModel();
    return true;
}

#endif // HEIMDALL_VAR_CHOICE_MODEL_HPP
