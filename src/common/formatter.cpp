/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "common/formatter.h"

#include "fs/util/fsutil.h"

#include "atools.h"
#include "fs/util/coordinates.h"
#include "unit.h"
#include "util/htmlbuilder.h"
#include "geo/pos.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QLocale>
#include <QRegularExpression>

namespace formatter {

static QStringList dateTimeFormats;

QString formatMinutesHours(double time)
{
  int hours = static_cast<int>(time);
  int minutes = atools::roundToInt((time - hours) * 60.);
  if(minutes == 60)
  {
    hours++;
    minutes = 0;
  }
  return QString(QObject::tr("%1:%2")).arg(QLocale().toString(hours)).
         arg(minutes, 2, 10, QChar('0'));
}

QString formatMinutesHoursLong(double time)
{
  int hours = static_cast<int>(time);
  int minutes = atools::roundToInt((time - hours) * 60);
  if(minutes == 60)
  {
    hours++;
    minutes = 0;
  }

  return QString(QObject::tr("%1 h %2 m")).arg(QLocale().toString(hours)).
         arg(minutes, 2, 10, QChar('0'));
}

QString formatMinutesHoursDays(double time)
{
  int days = static_cast<int>(time) / 24;
  int hours = static_cast<int>(time) - (days * 24);
  int minutes = atools::roundToInt((time - std::floor(time)) * 60.);
  return QString(QObject::tr("%1:%2:%3")).
         arg(QLocale().toString(days)).
         arg(hours, 2, 10, QChar('0')).
         arg(minutes, 2, 10, QChar('0'));
}

QString formatMinutesHoursDaysLong(double time)
{
  int days = static_cast<int>(time) / 24;
  int hours = static_cast<int>(time) - (days * 24);
  int minutes = atools::roundToInt((time - std::floor(time)) * 60.);
  QString retval;
  if(days > 0)
    retval += QString(QObject::tr("%1 d")).arg(QLocale().toString(days));

  if(hours > 0)
  {
    if(!retval.isEmpty())
      retval += QString(QObject::tr(" %1 h")).arg(hours, 2, 10, QChar('0'));
    else
      retval += QString(QObject::tr("%1 h")).arg(QLocale().toString(hours));
  }

  if(!retval.isEmpty())
    retval += QString(QObject::tr(" %1 m")).arg(minutes, 2, 10, QChar('0'));
  else
    retval += QString(QObject::tr("%1 m")).arg(QLocale().toString(minutes));

  return retval;
}

QString formatFloatUnit(float value, const QString& unit, int precision)
{
  if(unit.isEmpty())
    return QString(QObject::tr("%L1")).arg(QLocale().toString(value, 'f', precision));
  else
    return QString(QObject::tr("%L1 %2")).arg(QLocale().toString(value, 'f', precision)).arg(unit);
}

QString formatDoubleUnit(double value, const QString& unit, int precision)
{
  if(unit.isEmpty())
    return QString(QObject::tr("%L1")).arg(QLocale().toString(value, 'f', precision));
  else
    return QString(QObject::tr("%L1 %2")).arg(QLocale().toString(value, 'f', precision)).arg(unit);
}

QString formatDate(int timeT)
{
  QDateTime dateTime;
  dateTime.setTimeSpec(Qt::UTC);
  dateTime.setTime_t(static_cast<uint>(timeT));
  if(timeT > 0 && dateTime.isValid() && !dateTime.isNull())
    return dateTime.toString(Qt::DefaultLocaleShortDate);
  else
    return QObject::tr("Invalid date");
}

QString formatDateLong(int timeT)
{
  QDateTime dateTime;
  dateTime.setTimeSpec(Qt::UTC);
  dateTime.setTime_t(static_cast<uint>(timeT));
  if(timeT > 0 && dateTime.isValid() && !dateTime.isNull())
    // Workaround to remove the UTC label since FSX stores local time without timezone spec
    return dateTime.toString(Qt::DefaultLocaleLongDate).replace(QObject::tr("UTC"), "");
  else
    return QObject::tr("Invalid date");
}

QString formatElapsed(const QElapsedTimer& timer)
{
  int secs = static_cast<int>(timer.elapsed() / 1000L);
  if(secs < 60)
    return QObject::tr("%L1 %2").arg(secs).arg(secs == 1 ? QObject::tr("second") : QObject::tr("seconds"));
  else
  {
    int mins = secs / 60;
    secs = secs - mins * 60;

    return QObject::tr("%L1 %2 %L3 %4").
           arg(mins).arg(mins == 1 ? QObject::tr("minute") : QObject::tr("minutes")).
           arg(secs).arg(secs == 1 ? QObject::tr("second") : QObject::tr("seconds"));
  }
}

QString capNavString(const QString& str)
{
  return atools::fs::util::capNavString(str);
}

bool checkCoordinates(QString& message, const QString& text)
{
  Q_UNUSED(text);

  atools::geo::Pos pos = atools::fs::util::fromAnyFormat(text);

  if(pos.isValid())
  {
    QString coords = Unit::coords(pos);
    if(coords != text)
      message = QObject::tr("Coordinates are valid: %1").arg(coords);
    else
      // Same as in line edit. No need to show again
      message = QObject::tr("Coordinates are valid.");
    return true;
  }
  else
    // Show red warning
    message = atools::util::HtmlBuilder::errorMessage(QObject::tr("Coordinates are not valid."));
  return false;
}

QString yearVariant(QString dateTimeFormat)
{
  if(dateTimeFormat.contains("yyyy"))
    return dateTimeFormat.replace("yyyy", "yy");
  else if(dateTimeFormat.contains(QRegularExpression("\\byy\\b")))
    return dateTimeFormat.replace("yy", "yyyy");

  return dateTimeFormat;
}

void initTranslateableTexts()
{
  // Create a list of date time formats for system and English locales with all formats,
  // long short year variants and with time zone or not

  // System locale ==================================
  // This is independent from locale overridden in the options dialog
  QLocale locale = QLocale::system();
  dateTimeFormats.clear();
  dateTimeFormats.append(locale.dateTimeFormat(QLocale::ShortFormat));
  dateTimeFormats.append(locale.dateTimeFormat(QLocale::LongFormat));
  dateTimeFormats.append(locale.dateTimeFormat(QLocale::NarrowFormat));

  // Replace yyyy with yy and vice versa
  dateTimeFormats.append(yearVariant(locale.dateTimeFormat(QLocale::ShortFormat)));
  dateTimeFormats.append(yearVariant(locale.dateTimeFormat(QLocale::LongFormat)));
  dateTimeFormats.append(yearVariant(locale.dateTimeFormat(QLocale::NarrowFormat)));

  // English locale ==================================
  QLocale localeEn = QLocale::English;
  dateTimeFormats.append(localeEn.dateTimeFormat(QLocale::ShortFormat));
  dateTimeFormats.append(localeEn.dateTimeFormat(QLocale::LongFormat));
  dateTimeFormats.append(localeEn.dateTimeFormat(QLocale::NarrowFormat));

  // Replace yyyy with yy and vice versa
  dateTimeFormats.append(yearVariant(localeEn.dateTimeFormat(QLocale::ShortFormat)));
  dateTimeFormats.append(yearVariant(localeEn.dateTimeFormat(QLocale::LongFormat)));
  dateTimeFormats.append(yearVariant(localeEn.dateTimeFormat(QLocale::NarrowFormat)));

  // Add variants with time zone ===========================
  QStringList temp(dateTimeFormats);
  for(const QString& t : temp)
  {
    if(!t.endsWith("t"))
    {
      dateTimeFormats.append(t + " t");
      dateTimeFormats.append(t + "t");
    }
  }
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << dateTimeFormats;
#endif
}

QDateTime readDateTime(QString str)
{
  QDateTime retval;

  // This is independent from locale overridden in the options dialog
  QLocale locale = QLocale::system();

  // if(str.endsWith("UTC"))
  // str.chop(3);

  str = str.simplified();

  for(const QString& format : dateTimeFormats)
  {
    retval = locale.toDateTime(str, format);
    if(retval.isValid())
      break;
  }
  return retval;
}

} // namespace formatter
