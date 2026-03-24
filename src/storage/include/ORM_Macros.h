#pragma once

#include <QMetaObject>
#include <QtSql/QSqlQuery>
#include <QVariant>
#include <QStringList>
#include <QMetaProperty>

#define ORM_MODEL(Class, Table) \
public: \
    static QString tableName() { return Table; } \
    static QStringList fields() { \
        static QStringList f = []{ \
                  const QMetaObject *mo = &Class::staticMetaObject; \
                  QStringList list; \
                  for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i) \
                  list << QString::fromUtf8(mo->property(i).name()); \
                  return list; \
          }(); \
        return f; \
} \
    static QString primaryKey() { \
        static QString pk = []{ \
                  QStringList fs = fields(); \
                  return fs.isEmpty() ? QString() : fs.first(); \
          }(); \
        return pk; \
} \
    static Class fromSqlQuery(const QSqlQuery& query) { \
        Class obj; \
        const QMetaObject *mo = &Class::staticMetaObject; \
        int i = 0; \
        for (const QString& f : fields()) { \
            QMetaProperty prop = mo->property(mo->indexOfProperty(f.toUtf8())); \
            if (prop.isValid()) { \
                /* 直接写入 QVariant，保留 NULL 状态 */ \
                prop.writeOnGadget(&obj, query.value(i)); \
        } \
            i++; \
    } \
        return obj; \
} \
    QVariant getField(const QString& name) const { \
        const QMetaObject *mo = &Class::staticMetaObject; \
        QMetaProperty prop = mo->property(mo->indexOfProperty(name.toUtf8())); \
        return prop.isValid() ? prop.readOnGadget(this) : QVariant(); \
} \
    void setField(const QString& name, const QVariant& val) { \
        const QMetaObject *mo = &Class::staticMetaObject; \
        QMetaProperty prop = mo->property(mo->indexOfProperty(name.toUtf8())); \
        if (prop.isValid()) { \
            prop.writeOnGadget(this, val); \
    } \
} \
    private:

#define ORM_FIELD(type, name) \
              Q_PROPERTY(QVariant name READ name WRITE set##name) \
    public: \
    QVariant name() const { return m_##name; } \
    void set##name(const QVariant& val) { m_##name = val; } \
    bool is##name##Null() const { return m_##name.isNull(); } \
    type name##Value() const { return m_##name.value<type>(); } \
    private: \
    QVariant m_##name;
