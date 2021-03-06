// storage_engine.h

/**
*    Copyright (C) 2014 MongoDB Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*    As a special exception, the copyright holders give permission to link the
*    code of portions of this program with the OpenSSL library under certain
*    conditions as described in each individual source file and distribute
*    linked combinations including the program with the OpenSSL library. You
*    must comply with the GNU Affero General Public License in all respects for
*    all of the code used other than as permitted herein. If you modify file(s)
*    with this exception, you may extend this exception to your version of the
*    file(s), but you are not obligated to do so. If you do not wish to do so,
*    delete this exception statement from your version. If you delete this
*    exception statement from all source files in the program, then also delete
*    it in the license file.
*/

#pragma once

#include <string>
#include <vector>

#include "mongo/base/status.h"

namespace mongo {

    class DatabaseCatalogEntry;
    class OperationContext;
    class RecoveryUnit;
    struct StorageGlobalParams;

    /**
     * The StorageEngine class is the top level interface for creating a new storage
     * engine.  All StorageEngine(s) must be registered by calling registerFactory in order
     * to possibly be activated.
     */
    class StorageEngine {
    public:
        //
        // Static methods for registering StorageEngine implementations.  XXX: global config
        //
        class Factory {
        public:
            virtual ~Factory() { }
            virtual StorageEngine* create( const StorageGlobalParams& params ) const = 0;
        };

        static void registerFactory( const std::string& name, const Factory* factory );

        /**
         * Returns a new interface to the storage engine's recovery unit.  The recovery
         * unit is the durability interface.  For details, see recovery_unit.h
         *
         * Caller owns the returned pointer.
         */
        virtual RecoveryUnit* newRecoveryUnit( OperationContext* opCtx ) = 0;

        /**
         * List the databases stored in this storage engine.
         *
         * XXX: why doesn't this take OpCtx?
         */
        virtual void listDatabases( std::vector<std::string>* out ) const = 0;

        /**
         * Return the DatabaseCatalogEntry that describes the database indicated by 'db'.
         *
         * StorageEngine owns returned pointer.
         * It should not be deleted by any caller.
         */
        virtual DatabaseCatalogEntry* getDatabaseCatalogEntry( OperationContext* opCtx,
                                                               const StringData& db ) = 0;

        /**
         * Closes all file handles associated with a database.
         */
        virtual Status closeDatabase( OperationContext* txn, const StringData& db ) = 0;

        /**
         * Deletes all data and metadata for a database.
         */
        virtual Status dropDatabase( OperationContext* txn, const StringData& db ) = 0;

        /**
         * @return number of files flushed
         */
        virtual int flushAllFiles( bool sync ) = 0;

        virtual Status repairDatabase( OperationContext* txn,
                                       const std::string& dbName,
                                       bool preserveClonedFilesOnFailure = false,
                                       bool backupOriginalFiles = false ) = 0;

        /**
         * This method will be called before there is a clean shutdown.  Storage engines should
         * override this method if they have clean-up to do that is different from unclean shutdown.
         *
         * There is intentionally no uncleanShutdown().
         */
        virtual void cleanShutdown(OperationContext* txn) {}

    protected:
        /**
         * The destructor will never be called. See cleanShutdown instead.
         */
        virtual ~StorageEngine() {}
    };

    /**
     * Sets up the globalStorageEngine pointer and performs any startup work needed by the selected
     * storage engine. This must be called at a point where it is safe to spawn worker threads.
     */
    void initGlobalStorageEngine();

    // TODO: this is temporary
    extern StorageEngine* globalStorageEngine;
}
