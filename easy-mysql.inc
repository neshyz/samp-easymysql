#if defined _easymysql_included
    #endinput
#endif

#define _easymysql_included

#if !defined _logger_included
    #tryinclude <logger>
    #tryinclude "lib/easylogs/logger.inc"
#endif

#if !defined _logger_included
    #error "install easylogs.inc ->   neshyz/easylogs.git"
#endif





// crear un buffer de queries global
#if !defined EZMYSQL_MAX_QUERY_BUFFER_SIZE
    #define EZMYSQL_MAX_QUERY_BUFFER_SIZE (2048)
#endif

#if !defined EZMYSQL_MAX_ERROR_BUFFER_SIZE
    #define EZMYSQL_MAX_ERROR_BUFFER_SIZE (512)
#endif

static s_QueryBuffer[EZMYSQL_MAX_QUERY_BUFFER_SIZE];
static s_ErrorBuffer[EZMYSQL_MAX_ERROR_BUFFER_SIZE];


// tamaño buffer de conexiones mysql
#if !defined EZMYSQL_MAX_CONNECTIONS
    #define EZMYSQL_MAX_CONNECTIONS (3)
#endif


// tamaños de buffer parametros de conexion mysql
#if !defined EZMYSQL_MAX_PASS_SIZE
    #define EZMYSQL_MAX_PASS_SIZE (32)
#endif

#if !defined EZMYSQL_MAX_DBNAME_SIZE
    #define EZMYSQL_MAX_DBNAME_SIZE (16)
#endif

#if !defined EZMYSQL_MAX_USER_SIZE
    #define EZMYSQL_MAX_USER_SIZE (16)
#endif

#if !defined EZMYSQL_MAX_HOST_SIZE
    #define EZMYSQL_MAX_HOST_SIZE (16)
#endif



// definir loglevel de ezmysql
#if !defined EZMYSQL_LOG_LEVEL
    #define EZMYSQL_LOG_LEVEL (LOG_WARN)
#endif

// definir consoleappend de ezmysql
#if !defined EZMYSQL_LOG_CONSOLE
    #define EZMYSQL_LOG_CONSOLE (LOG_ERROR)
#endif

// definir fileappend de ezmysql
#if !defined EZMYSQL_LOG_FILE
    #define EZMYSQL_LOG_FILE (LOG_ALL)
#endif


#if !defined EZMYSQL_FORCE_CRASH
    #define EZMYSQL_FORCE_CRASH (true)
#endif


static enum E_MYSQL_CONNECTION_DATA {
    E_MYSQL_CONNECTION_DBNAME[EZMYSQL_MAX_DBNAME_SIZE char],
    E_MYSQL_CONNECTION_PASSWORD[EZMYSQL_MAX_PASS_SIZE char],
    E_MYSQL_CONNECTION_USER[EZMYSQL_MAX_USER_SIZE char],
    E_MYSQL_CONNECTION_HOST[EZMYSQL_MAX_HOST_SIZE char]
}

static s_ConnectionData[MySQL:EZMYSQL_MAX_CONNECTIONS][E_MYSQL_CONNECTION_DATA];

static Logger:s_Logger = INVALID_LOGGER_ID;














stock ezmysql_init()
{
    s_Logger = GetLogger("ezmysql", .fileAppend = EZMYSQL_LOG_FILE, .consoleAppend = EZMYSQL_LOG_CONSOLE, .level = EZMYSQL_LOG_LEVEL);

    if(!IsValidLogger(s_Logger)) {
        return 0;
    }
    else {
        return 1;
    }
}







stock MySQL:ezmysql_connect(const host[], const user[], const password[], const dbname[], MySQLOpt:option_id = MySQLOpt:0)
{
    new MySQL:handle = mysql_connect(host, user, password, dbname, option_id);

    if(handle != MYSQL_INVALID_HANDLE && mysql_errno(handle) == 0) {
        strpack(s_ConnectionData[handle][E_MYSQL_CONNECTION_HOST], host);
        strpack(s_ConnectionData[handle][E_MYSQL_CONNECTION_USER], user);
        strpack(s_ConnectionData[handle][E_MYSQL_CONNECTION_PASSWORD], password);
        strpack(s_ConnectionData[handle][E_MYSQL_CONNECTION_DBNAME], dbname);

        sendlog(
            s_Logger, LOG_INFO,
            "successful connection",
            logger_int("handle", _:handle),
            logger_string("host", host),
            logger_string("user", user),
            logger_string("password", password),
            logger_string("host", host)
        );

        return handle;
    }
    else {
        mysql_error(s_ErrorBuffer, .handle = handle);
        
        sendlog(s_Logger, LOG_FATAL, "failed to connect",
            logger_int("handle", _:handle),
            logger_string("host", host),
            logger_string("user", user),
            logger_string("password", password),
            logger_string("host", host),
            logger_string("error", s_ErrorBuffer)
        );


        #if EZMYSQL_FORCE_CRASH == true
            crash();
        #endif

        return MYSQL_INVALID_HANDLE;
    }
}






stock mysql_addtable(MySQL:handle, const tableName[], const entry[] = "ID INTEGER AUTO_INCREMENT, PRIMARY KEY(ID)")
{
    mysql_format(handle, s_QueryBuffer, sizeof(s_QueryBuffer),
        "CREATE TABLE IF NOT EXISTS `%s` (\
            %s);", tableName, entry
	);
        
    mysql_tquery(handle, s_QueryBuffer, "qcb_mysql_addtable", "iss", _:handle, tableName, entry);
}


forward qcb_mysql_addtable(MySQL:handle, const tableName[], const entry[]);
public qcb_mysql_addtable(MySQL:handle, const tableName[], const entry[])
{
    sendlog(s_Logger, LOG_TRACE,
        "table creation attempt",
        logger_int("handle", _:handle),
        logger_string("tableName", tableName),
        logger_string("entry", entry)
    );

    if(mysql_errno(handle) != 0) {

        mysql_error(s_ErrorBuffer);

        sendlog(s_Logger, LOG_ERROR,
            "table creation fail",
            logger_string("tableName", tableName),
            logger_string("entry", entry),
            logger_string("error", s_ErrorBuffer)

        );

    }
    else {

        sendlog(s_Logger, LOG_INFO,
            "table created",
            logger_string("tableName", tableName),
            logger_string("entry", entry)

        );
    }
}








stock mysql_columnexists(MySQL:handle, const table[], const columnName[])
{
    new dbname[EZMYSQL_MAX_DBNAME_SIZE];
    strunpack(dbname, s_ConnectionData[handle][E_MYSQL_CONNECTION_DBNAME]);

    mysql_format(handle, s_QueryBuffer, sizeof(s_QueryBuffer),
        "SELECT COUNT(*) AS column_exists \
        FROM information_schema.COLUMNS \
        WHERE TABLE_NAME = '%e' \
        AND TABLE_SCHEMA = '%e' \
        AND COLUMN_NAME = '%e';",
        table,
        dbname,
        columnName
    );

    mysql_query(handle, s_QueryBuffer, true);

    new int;
    cache_get_value_name_int(0, "column_exists", int);

    return int;
}

stock mysql_addcolumn(MySQL:handle, const table[], const columnName[], const entry[])
{
    sendlog(s_Logger, LOG_TRACE,
        "column creation attempt",
        logger_int("handle", _:handle),
        logger_string("tableName", table),
        logger_string("columnName", columnName),
        logger_string("entry", entry)
    );


    if(mysql_columnexists(handle, table, columnName)) {

        sendlog(s_Logger, LOG_TRACE,
            "column already exists",
            logger_int("handle", _:handle),
            logger_string("tableName", table),
            logger_string("columnName", columnName),
            logger_string("entry", entry)
        );

        return;
        
    }

    format(s_QueryBuffer, sizeof(s_QueryBuffer),

        "ALTER TABLE `%s` ADD COLUMN `%s` %s;\n",
        table, columnName, entry

    );

    mysql_tquery(handle, s_QueryBuffer, "qcb_mysql_addcolumn", "isss", _:handle, table, columnName, entry);
}

forward qcb_mysql_addcolumn(MySQL:handle, const table[], const columnName[], const entry[]);
public qcb_mysql_addcolumn(MySQL:handle, const table[], const columnName[], const entry[])
{
    if(mysql_errno(handle) != 0) {

        mysql_error(s_ErrorBuffer);

        sendlog(s_Logger, LOG_ERROR,
            "mysql column creation failed",
            logger_int("handle", _:handle),
            logger_string("table", table),
            logger_string("columnName", columnName),
            logger_string("entry", entry),
            logger_string("error", s_ErrorBuffer)
        );

    }
    else {

        sendlog(s_Logger, LOG_INFO,
            "mysql column created",
            logger_int("handle", _:handle),
            logger_string("table", table),
            logger_string("columnName", columnName),
            logger_string("entry", entry)
        );

    }
}







stock mysql_removecolumn(MySQL:handle, const tableName[], const columnName[])
{
    if(!mysql_columnexists(handle, tableName, columnName)) {

        return;
        
    }


    format(s_QueryBuffer, sizeof(s_QueryBuffer),

        "ALTER TABLE `%s` DROP COLUMN `%s`;\n",
        tableName, columnName

    );

    mysql_tquery(handle, s_QueryBuffer, "qcb_ezmysql_removecolumn", "iss", _:handle, tableName, columnName);
}





forward qcb_ezmysql_removecolumn(MySQL:handle, const tableName[], const columnName[]);
public qcb_ezmysql_removecolumn(MySQL:handle, const tableName[], const columnName[])
{
    sendlog(s_Logger, LOG_TRACE,
        "column removal attempt",
        logger_int("handle", _:handle),
        logger_string("tableName", tableName),
        logger_string("columnName", columnName)
    );
    
    if(mysql_errno(handle) != 0) {

        mysql_error(s_ErrorBuffer);

        sendlog(s_Logger, LOG_ERROR,
            "mysql column removal failed",
            logger_int("handle", _:handle),
            logger_string("table", tableName),
            logger_string("columnName", columnName),
            logger_string("error", s_ErrorBuffer)

        );

    }
    else {
        sendlog(s_Logger, LOG_INFO,
            "mysql column removal success",
            logger_int("handle", _:handle),
            logger_string("table", tableName),
            logger_string("columnName", columnName)
        );
    }
}


static stock crash()
{
    new File:fileThatDoesNotExist = fopen("idonotexist3948");
    fwrite(fileThatDoesNotExist, "");
}