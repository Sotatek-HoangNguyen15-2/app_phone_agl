/*
 * Copyright (C) 2017 Konsulko Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <QDebug>
#include <QObject>
#include <QTimer>
#include <telephony.h>
#include "phone.h"

Phone::Phone(Telephony *telephony, FilePlayer *player, QObject *parent) :
	QObject(parent),
	m_ringtone(player)
{
	QObject::connect(telephony, &Telephony::callStateChanged, this, &Phone::onCallStateChanged);

	m_call_timer.setInterval(1000);
	m_call_timer.setSingleShot(false);
	connect(&m_call_timer, SIGNAL(timeout()), this, SLOT(updateElapsedTime()));
}

void Phone::onCallStateChanged(QString callState)
{
	if (callState == "disconnected") {
		if (m_ringtone)
			m_ringtone->stop();
		m_call_timer.stop();
	} else if (callState == "active") {
		if (m_ringtone)
			m_ringtone->stop();
		m_date_time = m_date_time.fromSecsSinceEpoch(0);
		setElapsedTime("00:00:00");
		m_call_timer.start();
	} else if (callState == "incoming") {
		if (m_ringtone)
			m_ringtone->play(true);
	}
}

void Phone::updateElapsedTime() {
	m_date_time = m_date_time.addSecs(1);
	setElapsedTime(m_date_time.toString(Qt::ISODate).right(8));
}
