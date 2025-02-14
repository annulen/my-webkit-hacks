/*
 * Copyright (C) 2007 Staikos Computing Services Inc.
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Apple, Inc. All rights reserved.
 * Copyright (C) 2008 Collabora, Ltd. All rights reserved.
 * Copyright (C) 2010 Sencha, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FileSystem.h"

#include "FileMetadata.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#if !defined(Q_OS_WIN)
#include <sys/statvfs.h>
#endif

namespace WebCore {

bool fileExists(const String& path)
{
    return QFile::exists(path);
}


bool deleteFile(const String& path)
{
    return QFile::remove(path);
}

bool deleteEmptyDirectory(const String& path)
{
    return QDir::root().rmdir(path);
}

bool getFileSize(const String& path, long long& result)
{
    QFileInfo info(path);
    result = info.size();
    return info.exists();
}

bool getFileModificationTime(const String& path, time_t& result)
{
    QFileInfo info(path);
    result = info.lastModified().toTime_t();
    return info.exists();
}

bool getFileCreationTime(const String& path, time_t& result)
{
    QFileInfo info(path);
    result = info.created().toTime_t();
    return info.exists();
}

bool getFileMetadata(const String& path, FileMetadata& result)
{
    QFileInfo info(path);
    if (!info.exists())
        return false;
    result.modificationTime = info.lastModified().toTime_t();
    result.length = info.size();
    result.type = info.isDir() ? FileMetadata::Type::Directory : FileMetadata::Type::File;
    return true;
}

bool makeAllDirectories(const String& path)
{
    return QDir::root().mkpath(path);
}

String pathByAppendingComponent(const String& path, const String& component)
{
    return QDir::toNativeSeparators(QDir(path).filePath(component));
}

String homeDirectoryPath()
{
    return QDir::homePath();
}

String pathGetFileName(const String& path)
{
    return QFileInfo(path).fileName();
}

String directoryName(const String& path)
{
    return QFileInfo(path).absolutePath();
}

Vector<String> listDirectory(const String& path, const String& filter)
{
    Vector<String> entries;

    QStringList nameFilters;
    if (!filter.isEmpty())
        nameFilters.append(filter);
    QFileInfoList fileInfoList = QDir(path).entryInfoList(nameFilters, QDir::AllEntries | QDir::NoDotAndDotDot);
    foreach (const QFileInfo fileInfo, fileInfoList) {
        String entry = String(fileInfo.canonicalFilePath());
        entries.append(entry);
    }

    return entries;
}

CString fileSystemRepresentation(const String& path)
{
    // FIXME: Check if we need to handle escaping or encoding issues here
    //
    // Right now this function is used in SQLiteFileSystem::openDatabase where UTF8
    // is *required* by sqlite_open, but it may need to return local encoding in other
    // circumstances
    return path.utf8();
}

String stringFromFileSystemRepresentation(const char* fileSystemRepresentation)
{
    // This needs to do the opposite of fileSystemRepresentation.
    return String::fromUTF8(fileSystemRepresentation);
}

String openTemporaryFile(const String& prefix, FileSystem::PlatformFileHandle& handle)
{
#ifndef QT_NO_TEMPORARYFILE
    QTemporaryFile* tempFile = new QTemporaryFile(QDir::tempPath() + QLatin1Char('/') + QString(prefix));
    tempFile->setAutoRemove(false);
    QFile* temp = tempFile;
    if (temp->open(QIODevice::ReadWrite)) {
        handle = temp;
        return temp->fileName();
    }
#endif
    handle = FileSystem::invalidPlatformFileHandle;
    return String();
}

FileSystem::PlatformFileHandle openFile(const String& path, FileSystem::FileOpenMode mode)
{
    QIODevice::OpenMode platformMode;

    if (mode == FileSystem::FileOpenMode::Read)
        platformMode = QIODevice::ReadOnly;
    else if (mode == FileSystem::FileOpenMode::Write)
        platformMode = (QIODevice::WriteOnly | QIODevice::Truncate);
    else
        return FileSystem::invalidPlatformFileHandle;

    QFile* file = new QFile(path);
    if (file->open(platformMode))
        return file;

    return FileSystem::invalidPlatformFileHandle;
}

int readFromFile(FileSystem::PlatformFileHandle handle, char* data, int length)
{
    if (handle && handle->exists() && handle->isReadable())
        return handle->read(data, length);
    return 0;
}

void closeFile(FileSystem::PlatformFileHandle& handle)
{
    if (handle) {
        handle->close();
        delete handle;
    }
}

long long seekFile(FileSystem::PlatformFileHandle handle, long long offset, FileSystem::FileSeekOrigin origin)
{
    if (handle) {
        long long current = 0;

        switch (origin) {
        case FileSystem::FileSeekOrigin::Beginning:
            break;
        case FileSystem::FileSeekOrigin::Current:
            current = handle->pos();
            break;
        case FileSystem::FileSeekOrigin::End:
            current = handle->size();
            break;
        }

        // Add the offset to the current position and seek to the new position
        // Return our new position if the seek is successful
        current += offset;
        if (handle->seek(current))
            return current;
        else
            return -1;
    }

    return -1;
}

uint64_t getVolumeFreeSizeForPath(const char* path)
{
#if defined(Q_OS_WIN)
    ULARGE_INTEGER freeBytesToCaller;
    BOOL result = GetDiskFreeSpaceExW((LPCWSTR)path, &freeBytesToCaller, 0, 0);
    if (!result)
        return 0;
    return static_cast<uint64_t>(freeBytesToCaller.QuadPart);
#else
    struct statvfs volumeInfo;
    if (statvfs(path, &volumeInfo))
        return 0;

    return static_cast<uint64_t>(volumeInfo.f_bavail) * static_cast<uint64_t>(volumeInfo.f_frsize);
#endif
}


int writeToFile(FileSystem::PlatformFileHandle handle, const char* data, int length)
{
    if (handle && handle->exists() && handle->isWritable())
        return handle->write(data, length);

    return 0;
}

// QTFIXME: Move elsewhere?
bool unloadModule(PlatformModule module)
{
#if defined(Q_OS_MACOS)
    CFRelease(module);
    return true;

#elif defined(Q_OS_WIN)
    return ::FreeLibrary(module);

#else
#ifndef QT_NO_LIBRARY
    if (module->unload()) {
        delete module;
        return true;
    }
#endif
    return false;
#endif
}

}

// vim: ts=4 sw=4 et
