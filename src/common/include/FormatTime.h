#ifndef FORMATTIME_H
#define FORMATTIME_H

#include <QDateTime>

inline QString FormatTime(qint64 timestamp)
{
    if(timestamp <= 0) return QString();

    QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp);
    if(!dt.isValid()) return QString();

    QDateTime now = QDateTime::currentDateTime();

    if(dt.date() == now.date()){
        return dt.toString("HH:mm");
    }
    qint64 days = dt.daysTo(now);
    if(days < 7){
        switch(dt.date().dayOfWeek()) {
        case Qt::Monday: return "周一";
        case Qt::Tuesday: return "周二";
        case Qt::Wednesday: return "周三";
        case Qt::Thursday: return "周四";
        case Qt::Friday: return "周五";
        case Qt::Saturday: return "周六";
        case Qt::Sunday: return "周日";
        default: return QString();
        }
    }
    return dt.toString("yyyy-MM-dd");
}

#endif // FORMATTIME_H
