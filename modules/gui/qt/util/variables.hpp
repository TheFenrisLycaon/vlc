/*****************************************************************************
 * variables.hpp : Heimdall variable class
 ****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
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

#ifndef QHEIMDALL_VARIABLES_H_
#define QHEIMDALL_VARIABLES_H_ 1

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_variables.h>
#include <QObject>

#include "qt.hpp"
#include "varcommon_p.hpp"

/*
 * type traits to convert HEIMDALL_VAR to C++types
 */
template<typename T>
struct HeimdallVarTypeTraits {
};

template<>
struct HeimdallVarTypeTraits<QString>
{
    static const int var_type = HEIMDALL_VAR_STRING;
    inline static QString fromValue(const heimdall_value_t& value) {
        return value.psz_string;
    }
    inline static heimdall_value_t toValue(const QString& value) {
        heimdall_value_t ret;
        ret.psz_string = strdup(qtu(value));
        return ret;
    }
    inline static void releaseValue(heimdall_value_t value) {
        free(value.psz_string);
    }
};

template<>
struct HeimdallVarTypeTraits<bool>
{
    static const int var_type = HEIMDALL_VAR_BOOL;
    inline static bool fromValue(const heimdall_value_t& value) {
        return value.b_bool;
    }
    inline static heimdall_value_t toValue(bool value) {
        heimdall_value_t ret;
        ret.b_bool = value;
        return ret;
    }
    inline static void releaseValue(heimdall_value_t) {}
};

template<>
struct HeimdallVarTypeTraits<int64_t>
{
    static const int var_type = HEIMDALL_VAR_INTEGER;
    inline static int64_t fromValue(const heimdall_value_t& value) {
        return value.i_int;
    }
    inline static heimdall_value_t toValue(int64_t value) {
        heimdall_value_t ret;
        ret.i_int = value;
        return ret;
    }
    inline static void releaseValue(heimdall_value_t) {}
};

template<>
struct HeimdallVarTypeTraits<float>
{
    static const int var_type = HEIMDALL_VAR_FLOAT;
    inline static float fromValue(const heimdall_value_t& value) {
        return value.f_float;
    }
    inline static heimdall_value_t toValue(float value) {
        heimdall_value_t ret;
        ret.f_float = value;
        return ret;
    }
    inline static void releaseValue(heimdall_value_t) {}
};

//Generic observer
template<typename Derived, typename BaseType>
class QHeimdallVariable : public QObject
{
    typedef QHeimdallVariable<Derived, BaseType> SelfType;

    struct QHeimdallVariableCRef  {
        SelfType* self;
    };
    static_assert( std::is_pod<QHeimdallVariableCRef>::value,  "QHeimdallVariableCRef must be POD");

public:
    template<typename T>
    QHeimdallVariable(T* object, const QString &property, QObject* parent)
        : QObject(parent)
        , m_object(new HeimdallObjectHolderImpl<T>(nullptr))
        , m_property(property)
    {
        cref.self = this;
        /* This parameter is later used by derivative classes, and only helps
         * the type inference here. */
        HEIMDALL_UNUSED(object);
    }

    virtual ~QHeimdallVariable()
    {
        assert(m_object->get() == nullptr);
    }

    ///change the object beeing observed
    template<typename T>
    void resetObject( T* object )
    {
        clearObject();
        if (object)
        {
            m_object->reset( object, true );
            int type = var_Type(object, qtu(m_property));
            if (type == 0) //variable not found
            {
                msg_Warn(m_object->get(), "variable %s not found in object", qtu(m_property));
                m_object->clear();
                return;
            }
            assert((type & HEIMDALL_VAR_CLASS) == HeimdallVarTypeTraits<BaseType>::var_type);
            heimdall_value_t currentvalue;
            if (var_Get(m_object->get(), qtu(m_property), &currentvalue) == HEIMDALL_SUCCESS)
            {
                Derived* derived = static_cast<Derived*>(this);
                m_value = HeimdallVarTypeTraits<BaseType>::fromValue(currentvalue);
                emit derived->valueChanged( m_value );
            }

            var_Create(m_object->get(), qtu(m_property), HeimdallVarTypeTraits<BaseType>::var_type);
            var_AddCallback(m_object->get(), qtu(m_property), value_modified, &cref);
        }
    }

    void clearObject()
    {
        if (m_object->get())
        {
            var_DelCallback( m_object->get(), qtu(m_property), value_modified, &cref );
            var_Destroy(m_object->get(), qtu(m_property));
            m_object->clear();
        }
    }

    BaseType getValue() const
    {
        if (!m_object->get())
            return BaseType{};
        return m_value;
    }

protected:
    //called by setValue in child classes
    void setValueInternal(BaseType value)
    {
        if (! m_object->get())
            return;
        heimdall_value_t heimdallvalue = HeimdallVarTypeTraits<BaseType>::toValue( value );
        var_Set(m_object->get(), qtu(m_property), heimdallvalue);
        HeimdallVarTypeTraits<BaseType>::releaseValue(heimdallvalue);
    }

    //executed on UI thread
    virtual void onValueChangedInternal(heimdall_object_t* object, BaseType value)
    {
        if (m_object->get() != object)
            return;
        if (m_value != value) {
            m_value = std::move(value);
            Derived* derived = static_cast<Derived*>(this);
            emit derived->valueChanged( m_value );
        }
    }


private:
    //executed on variable thread, this forwards the callback to the UI thread
    static int value_modified( heimdall_object_t * object, char const *, heimdall_value_t, heimdall_value_t newValue, void * data)
    {
        QHeimdallVariableCRef* cref = static_cast<QHeimdallVariableCRef*>(data);
        Derived* derived = static_cast<Derived*>(cref->self);
        emit derived->onValueChangedInternal( object, HeimdallVarTypeTraits<BaseType>::fromValue( newValue ) );
        return HEIMDALL_SUCCESS;
    }

    std::unique_ptr<HeimdallObjectHolder> m_object;
    QString m_property;
    BaseType m_value;
    QHeimdallVariableCRef cref;
};

//specialisation

class QHeimdallBool : public QHeimdallVariable<QHeimdallBool, bool>
{
    Q_OBJECT
public:
    Q_PROPERTY(bool value READ getValue WRITE setValue NOTIFY valueChanged FINAL)

    template<typename T>
    QHeimdallBool(T* object, const QString &property, QObject* parent = nullptr)
        : QHeimdallVariable<QHeimdallBool, bool>(object, property, parent)
    {
        resetObject<T>(object);
        connect(this, &QHeimdallBool::valueChangedInternal, this, &QHeimdallBool::onValueChangedInternal, Qt::QueuedConnection);
    }

    ~QHeimdallBool() {
        clearObject();
    }

public slots:
    void setValue(bool value);

signals:
    void valueChanged( bool );
    void valueChangedInternal(heimdall_object_t *, bool );
};

class QHeimdallString : public QHeimdallVariable<QHeimdallString, QString>
{
    Q_OBJECT
public:
    Q_PROPERTY(QString value READ getValue WRITE setValue NOTIFY valueChanged FINAL)

    template<typename T>
    QHeimdallString(T* object, const QString &property, QObject* parent = nullptr)
        : QHeimdallVariable<QHeimdallString, QString>(object, property, parent)
    {
        resetObject<T>(object);
        connect(this, &QHeimdallString::valueChangedInternal, this, &QHeimdallString::onValueChangedInternal, Qt::QueuedConnection);
    }

    ~QHeimdallString() {
        clearObject();
    }

public slots:
    void setValue(const QString &value);

signals:
    void valueChanged( QString );
    void valueChangedInternal( heimdall_object_t *, QString );
};

class QHeimdallFloat : public QHeimdallVariable<QHeimdallFloat, float>
{
    Q_OBJECT
public:
    Q_PROPERTY(float value READ getValue WRITE setValue NOTIFY valueChanged FINAL)

    template<typename T>
    QHeimdallFloat(T* object, const QString &property, QObject* parent = nullptr)
        : QHeimdallVariable<QHeimdallFloat, float>(object, property, parent)
    {
        resetObject<T>(object);
        connect(this, &QHeimdallFloat::valueChangedInternal, this, &QHeimdallFloat::onValueChangedInternal, Qt::QueuedConnection);
    }

    ~QHeimdallFloat() {
        clearObject();
    }

public slots:
    void setValue(float value);

signals:
    void valueChanged( float );
    void valueChangedInternal( heimdall_object_t *, float );
};


class QHeimdallInteger : public QHeimdallVariable<QHeimdallInteger, int64_t>
{
    Q_OBJECT
public:
    Q_PROPERTY(int64_t value READ getValue WRITE setValue NOTIFY valueChanged FINAL)

    template<typename T>
    QHeimdallInteger(T* object, const QString &property, QObject* parent = nullptr)
        : QHeimdallVariable<QHeimdallInteger, int64_t>(object, property, parent)
    {
        resetObject<T>(object);
        connect(this, &QHeimdallInteger::valueChangedInternal, this, &QHeimdallInteger::onValueChangedInternal, Qt::QueuedConnection);
    }

    ~QHeimdallInteger() {
        clearObject();
    }

public slots:
    void setValue(int64_t value);

signals:
    void valueChanged( int64_t );
    void valueChangedInternal( heimdall_object_t *,  int64_t );
};

#endif
