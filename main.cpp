#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCryptographicHash>
#include <iostream>
using namespace std;

void dfsSearch(const QRegularExpression& pattern, QString path, QSet<QString>& result) {
	if (QFileInfo(path).isDir()) {
		QDir dir(path);
		QStringList entry = dir.entryList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
		for (const auto& s : entry) {
			dfsSearch(pattern, path + QDir::separator() + s, result);
		}
	} else if (QFileInfo(path).isFile()) {
		if (pattern.match(path).hasMatch()) {
			result.insert(path.mid(2)); // remove './'
		}
	}
}

int main(int argc, char *argv[]) {
	QCoreApplication a(argc, argv);
	if (argc == 1 || !strcmp(argv[1], "update")) {
		// Update
		QSettings setting("Updater.ini");
		QString localDir = setting.value("local").toString();
		QString infoUrl = setting.value("info").toString();
		QNetworkAccessManager manager;
		QNetworkRequest request;
		request.setUrl(infoUrl);
		QNetworkReply* reply = manager.get(request);
		QObject::connect(reply, &QNetworkReply::finished, [&]() {
			QByteArray data = reply->readAll();
			cout << data.data() << endl;
			QJsonDocument document = QJsonDocument::fromBinaryData(data);
			if (!document.isArray()) {
				cerr << "Updater: error parsing info.json";
				exit(1);
			}
		});
		return a.exec();
	} else if (!strcmp(argv[1], "generate")) {
		// Generate

		// TODO: save md5 and rehash if necessary
		QDir dir = QDir::current();
		QFile cfg("info.ucfg");
		cfg.open(QIODevice::ReadOnly);
		QTextStream stream(&cfg);
		QString rule;
		int line = 0;
		auto SyntaxError = [line](QString info = "") {
			cerr << "Updater: Syntax error at info.ucfg in line " << line << endl;
			if (info.length()) {
				cerr << "\terror info:" << info.toStdString() << endl;
			}
			exit(1);
		};
		QSet<QString> result;
		while (stream.readLineInto(&rule)) {
			++line;
			if (!rule.length() || rule.trimmed().length() == 0 || rule[0] == '#') {
				continue;
			}
			if (rule.length() < 2 || rule[1] != ' ') {
				SyntaxError();
			}
			if (rule[0] != '+' && rule[0] != '-') {
				SyntaxError();
			}
			QRegularExpression reg(rule.mid(2));
			if (!reg.isValid()) {
				SyntaxError(reg.errorString());
			}
			if (rule[0] == '+') {
				dfsSearch(reg, ".", result);
			} else {
				QSet<QString> temp;
				for (const auto& s : result) {
					if (!reg.match(s).hasMatch()) {
						temp.insert(s);
					}
				}
				temp.swap(result);
			}
		}
		QJsonDocument doc;
		QJsonArray arr;
		for (const auto& s : result) {
			cout << s.toStdString() << endl;
			QJsonObject info;
			info.insert("path", QJsonValue(s));
			info.insert("size", QJsonValue(QFileInfo(s).size()));
			QFile dat(s);
			dat.open(QIODevice::ReadOnly);
			cout << "hashing" << endl;
			info.insert("md5", QJsonValue(QString(QCryptographicHash::hash(dat.readAll(), QCryptographicHash::Md5).toHex())));
			arr.append(info);
		}
		doc.setArray(arr);
		QFile js("info.json");
		js.open(QIODevice::WriteOnly);
		js.write(doc.toJson(QJsonDocument::Compact));
	} else {
		cout << "Usage: " << argv[0] << " [command]" << endl;
		cout << "command:" << endl;
		cout << "\tupdate : update project" << endl;
		cout << "\tgenerate: generate info.json" << endl;
		a.quit();
		return 0;
	}
}

