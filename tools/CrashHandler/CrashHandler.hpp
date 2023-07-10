//
// Created by aojoie on 6/1/2023.
//

#pragma once

#include <ojoie/Configuration/typedef.h>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace AN {

class CrashHandlerImpl;

typedef void (*TCrashCallback)(const char *crashFilesPath);

class CrashHandlerInterface {
public:

    virtual bool install() = 0;

    virtual void uninstall() = 0;

    virtual int processCrash(EXCEPTION_POINTERS *ep) = 0;

    virtual bool isInstalled() const = 0;

    virtual void setCrashCallback(TCrashCallback cb) = 0;

    virtual const char *getCrashReportFolder() = 0;
};

class CrashHandler : public CrashHandlerInterface, private NonCopyable {
    CrashHandlerImpl *impl;
public:

    CrashHandler(const char *crashReportAppPath,
                 const char *appName,
                 const char *version,
                 const char *crashReportFolder);

    ~CrashHandler();

    /**
	 *	Installs crash handler for the application.
	 *  @return true if succeeded, false otherwise
	 */
    virtual bool install() override;

    /**
	 *	Uninstalls crash handler from the application. This call is optional,
	 *  use it if you manually need to unhook the handler.
	 */
    virtual void uninstall() override;

    /**
	 *	Manually call crash processing.
	 */
    virtual int processCrash(EXCEPTION_POINTERS *ep) override;

    /**
	 *	Returns true if crash handler is installed currently.
	 */
    virtual bool isInstalled() const override;

    /**
	 *	Sets callback that is called when a crash occurs to do
	 *  any really important cleanup. NOTE: the callback should do
	 *  as little work as possible, because it's inside the crash
	 *  and we don't want extra crashes from inside the callback.
	 */
    virtual void setCrashCallback(TCrashCallback cb) override;

    virtual const char *getCrashReportFolder() override;
};

}// namespace AN

typedef AN::CrashHandlerInterface *(*PFN_ANCreateCrashHandler)(const char *crashReportAppPath,
                                                           const char *appName,
                                                           const char *version,
                                                           const char *crashReportFolder);

typedef void (*PFN_ANDeleteCrashHandler)(AN::CrashHandlerInterface *handler);


