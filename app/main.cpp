/*
 * Copyright (C) 2016 The Qt Company Ltd.
 * Copyright (C) 2017,2020 Konsulko Group
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

#include <QtCore/QDebug>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQuickControls2/QQuickStyle>
#include <QQuickWindow>

#include <pbap.h>
#include <telephony.h>
#include "phone.h"
#include "numbertype.h"
#include "fileplayer.h"

int main(int argc, char *argv[])
{
	QString graphic_role = QString("phone");

	QGuiApplication app(argc, argv);
	app.setDesktopFileName(graphic_role);

	QQuickStyle::setStyle("AGL");

	QQmlApplicationEngine engine;
	QQmlContext *context = engine.rootContext();
	Telephony *telephony = new Telephony();
	context->setContextProperty("telephony", telephony);

	std::string ringtone_dir("/usr/share/sounds/ringtones");
	FilePlayer *player = new FilePlayer(ringtone_dir + "/Phone.wav", std::string("phone"));
	Phone *phone = new Phone(telephony, player);
	context->setContextProperty("phone", phone);

	QObject::connect(telephony, &Telephony::callStateChanged, phone, &Phone::onCallStateChanged);
	context->setContextProperty("pbap", new Pbap(context));

	qmlRegisterUncreatableType<NumberType>("NumberType", 1, 0, "NumberType", "Not creatable as it is an enum type");
        engine.load(QUrl(QStringLiteral("qrc:/Phone.qml")));

	return app.exec();
}

