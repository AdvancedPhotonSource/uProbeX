/*-----------------------------------------------------------------------------
 * Copyright (c) 2014, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include <QApplication>

#include <core/uProbeX.h>
#include <QDateTime>
#include <QString>
#include <QFile>
#include <QMetaObject>
#include <QTextStream>
#include "mvc/LiveMapsElementsWidget.h"

#include <mutex>
#include <streambuf>

namespace
{

class ThreadSafeLogBuffer final : public std::streambuf
{
public:
    std::string take()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        std::string output;
        output.swap(_output);
        return output;
    }

protected:
    int_type overflow(int_type ch) override
    {
        if (traits_type::eq_int_type(ch, traits_type::eof()))
        {
            return traits_type::not_eof(ch);
        }

        std::lock_guard<std::mutex> lock(_mutex);
        _output.push_back(traits_type::to_char_type(ch));
        return ch;
    }

    std::streamsize xsputn(const char* text, std::streamsize count) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _output.append(text, static_cast<std::size_t>(count));
        return count;
    }

private:
    std::mutex _mutex;
    std::string _output;
};

ThreadSafeLogBuffer& consoleLogBuffer()
{
    // Keep the buffer alive through static destruction. main() restores cout's
    // original buffer before returning, but other static destructors may still log.
    static ThreadSafeLogBuffer* buffer = new ThreadSafeLogBuffer();
    return *buffer;
}

class ScopedCoutRedirect
{
public:
    explicit ScopedCoutRedirect(std::streambuf* replacement)
        : _original(std::cout.rdbuf(replacement))
    {
    }

    ~ScopedCoutRedirect()
    {
        std::cout.rdbuf(_original);
    }

private:
    std::streambuf* _original;
};

void appendLogHtmlOnGuiThread(QString html, bool ensureCursorVisible)
{
    QCoreApplication* application = QCoreApplication::instance();
    if (application == nullptr)
    {
        return;
    }

    QMetaObject::invokeMethod(application,
        [html = std::move(html), ensureCursorVisible]()
        {
            QTextEdit* log = uProbeX::log_textedit;
            if (log == nullptr)
            {
                return;
            }

            log->insertHtml(html);
            if (ensureCursorVisible)
            {
                log->ensureCursorVisible();
            }
        },
        Qt::QueuedConnection);
}

}

/*---------------------------------------------------------------------------*/

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{

	QString h_msg(msg);
	QByteArray localMsg = msg.toLocal8Bit();
	const char* contextFile = context.file != nullptr ? context.file : "unknown";
	const char* contextFunction = context.function != nullptr ? context.function : "unknown";
	int cnt = 0;	
	if (type == QtInfoMsg || type == QtDebugMsg)
	{
		if (type == QtInfoMsg)
		{
			fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), contextFile, context.line, contextFunction);
		}
		else if(type == QtDebugMsg)
		{
			fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), contextFile, context.line, contextFunction);
		}
		cnt += h_msg.count("Info: ");
		cnt += h_msg.count("Warning: ");
		cnt += h_msg.count("Error: ");
		h_msg.replace("Info: ", "<span style=\" font-weight: bold; \">Info: </span>");
		h_msg.replace("\033[1;31mError: \033[0;m", "<span style=\"color : red; font-weight: bold; \">Error: </span>");
		h_msg.replace("\033[1;33mWarning: \033[0;m", "<span style=\"color : yellow; font-weight: bold; \">Warning: </span>");
		if (cnt > 0)
		{
			h_msg.replace("\n", "<br>");
		}
		appendLogHtmlOnGuiThread(h_msg, true);
	}
	else if (type == QtWarningMsg)
	{
		fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), contextFile, context.line, contextFunction);
		h_msg = "<span style=\"color : yellow; \">" + msg + "</span><br />";
		appendLogHtmlOnGuiThread(h_msg, false);
	}
	else if (type == QtCriticalMsg)
	{
		fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), contextFile, context.line, contextFunction);
		h_msg = "<span style=\"color : red; \">" + msg + "</span><br />";
		appendLogHtmlOnGuiThread(h_msg, false);
	}
	else if (type == QtFatalMsg)
	{
		fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), contextFile, context.line, contextFunction);
	}
	else
	{
		fprintf(stderr, "Unknown: %s (%s:%u, %s)\n", localMsg.constData(), contextFile, context.line, contextFunction);
	}
	
}

/*---------------------------------------------------------------------------*/
// This redirects console logs from DLL's to out log widget
void readSTDOUT()
{
	std::string line = consoleLogBuffer().take();
	if (line.length() > 0)
	{
		qDebug() << line.c_str();
	}
}

/*---------------------------------------------------------------------------*/

int main(int argc, char** argv)
{

	// check env vars for preferences file
	bool pref_changed = false;
	if (const char* env_pref_dir = std::getenv("UPROBEX_PREF_DIR"))
	{
		Preferences::inst()->set_default_dir(QString(env_pref_dir));
		pref_changed = true;
	}
	if (const char* env_pref_file = std::getenv("UPROBEX_PREF_FILE"))
	{
		Preferences::inst()->set_default_file(QString(env_pref_file));
		pref_changed = true;
	}
	if(pref_changed)
	{
		Preferences::inst()->load(true);
	}

	QApplication app(argc, argv);
	//dark style
	
	QString str_theme = Preferences::inst()->getValue(STR_PFR_THEME).toString();
	if (str_theme.length() > 0 && str_theme != STR_DEFAULT)
	{
		QString theme_file = "../themes/" + str_theme + ".qss";
		QFile* file = new QFile(theme_file);
		file->open(QFile::ReadOnly | QFile::Text);
		QTextStream* stream = new QTextStream(file);
		app.setStyleSheet(stream->readAll());
	}
	qInstallMessageHandler(myMessageOutput);

	ScopedCoutRedirect coutRedirect(&consoleLogBuffer());
	QTimer timer;
	QObject::connect(&timer, &QTimer::timeout, readSTDOUT);
	timer.start(1000);

	if(argc > 1)
	{
		QString arg1 = QString(argv[1]);
		if(arg1 == "--live")
		{
			LiveMapsElementsWidget *liveMapsViewer = new LiveMapsElementsWidget();
			liveMapsViewer->show();
			return app.exec();
		}
	}


    // get location of where we are running from and use it to find ref files
    std::string exe_loc = std::string(argv[0]);
	int prog_idx = exe_loc.find("MacOS/uProbeX"); //  MacOS
    if (prog_idx > 0)
    {
        exe_loc = exe_loc.substr(0, prog_idx);
    }
	else // Linux
	{
		prog_idx = exe_loc.find("bin/uProbeX");
		if (prog_idx > 0)
		{
			exe_loc = exe_loc.substr(0, prog_idx);
		}
		else // windows
		{
			prog_idx = exe_loc.find("bin\\uProbeX.exe");
			if (prog_idx > 0)
			{
				exe_loc = exe_loc.substr(0, prog_idx);
			}
			else
			{
				exe_loc = ".";
			}
		}
	}
    //////// HENKE and ELEMENT INFO /////////////
    const std::string element_csv_filename = exe_loc + "./reference/xrf_library.csv";
    const std::string element_henke_filename = exe_loc + "./reference/henke.xdr";
    const std::string scaler_lookup_yaml = exe_loc + "./reference/Scaler_to_PV_map.yaml";

    if (false == io::file::load_scalers_lookup(scaler_lookup_yaml))
    {
        logE << " Could not load " << scaler_lookup_yaml << ". Won't be able to translate from PV to Label for scalers!\n";
    }

    //load element information
    if (false == io::file::load_element_info<float>(element_henke_filename, element_csv_filename))
    {
        logE << "loading element information: " << "\n";
        return -1;
    }
    if (false == io::file::load_element_info<double>(element_henke_filename, element_csv_filename))
    {
        logE << "loading element information: " << "\n";
        return -1;
    }
    
//#if defined _WIN32 || defined __CYGWIN__
//	FreeConsole();
//#endif
	// Startup application
	uProbeX* widget = new uProbeX(nullptr);
	//widget->setAttribute(Qt::WA_DeleteOnClose);
	widget->setWindowState(Qt::WindowMaximized);
	widget->show();
	int ret = 0;
	try
	{
			// Run
		ret = app.exec();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}

	// Clean up
	if (widget != nullptr)
	{
		delete widget;
	}


	if(pref_changed)
	{
		Preferences::inst()->save();
	}

	return ret;
}

/*---------------------------------------------------------------------------*/
