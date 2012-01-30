#include "dbactionqueue.h"

#include "../include/afanasy.h"

#include "../libafanasy/environment.h"

#include "../libafsql/dbjob.h"
#include "../libafsql/dbrender.h"
#include "../libafsql/dbuser.h"

#include "afcommon.h"
#include "monitorcontainer.h"

extern bool running;

#define AFOUTPUT
#undef AFOUTPUT
#include "../include/macrooutput.h"

DBActionQueue::DBActionQueue( const std::string & i_name, MonitorContainer * i_monitorcontainer):
   AfQueue( i_name),
   m_monitors( i_monitorcontainer)
{
    m_conn = PQconnectdb( af::Environment::get_DB_ConnInfo().c_str());
    if( PQstatus( m_conn) != CONNECTION_OK)
    {
        printf("Database connection \"%s\" is not working.\n", name.c_str());
        m_working = false;
    }
    else
    {
        connectionEstablished();
        m_working = true;
    }
}

DBActionQueue::~DBActionQueue()
{
    if( m_conn )
    {
        PQfinish( m_conn);
    }
}

void DBActionQueue::connectionEstablished()
{
   AFINFA("DBActionQueue::connectionEstablished: %s", name.c_str())
}

void DBActionQueue::processItem( AfQueueItem* item)
{
//printf("DBActionQueue::processItem: %s:\n", name.c_str());
    if( false == m_working )
    {
        delete item;
        return;
    }
    if( PQstatus( m_conn) != CONNECTION_OK)
    {
        if( m_conn != NULL )
        {
            PQfinish( m_conn);
            m_conn = NULL;
        }
        for(;;)
        {
            if( false == running )
            {
                delete item;
                return;
            }
            m_conn = PQconnectdb( af::Environment::get_DB_ConnInfo().c_str());
            if( PQstatus( m_conn) == CONNECTION_OK)
            {
                connectionEstablished();
                sendConnected();
                break;
            }
            if( m_conn != NULL )
            {
                PQfinish( m_conn);
                m_conn = NULL;
            }
            sendAlarm();
            sleep( AFDATABASE::RECONNECTAFTER);
        }
    }

    // Writing an item and check if error:
    if( false == writeItem( item))
    {
        // Check if database has just closed:
        if( PQstatus( m_conn) != CONNECTION_OK)
        {
            if( m_conn != NULL )
            {
                PQfinish( m_conn);
                m_conn = NULL;
            }
            // Push item back to queue front to try it to write again next time:
            push( item, true );
            AFINFA("%s: Item pushed back to queue front.", name.c_str())
            return;
        }
    }
    delete item;
}

bool DBActionQueue::writeItem( AfQueueItem* item)
{
//printf("DBActionQueue::writeItem:\n");
   Queries * queries = (Queries*)item;

   int size = queries->size();
   if( size < 1) return true;

   return afsql::execute( m_conn, queries);
}

void DBActionQueue::addItem( const afsql::DBItem * item)
{
   Queries * queries = new Queries();
   item->dbInsert( queries);
   push( queries);
}

void DBActionQueue::updateItem( const afsql::DBItem * item, int attr)
{
   Queries * queries = new Queries();
   item->dbUpdate( queries, attr);
   push( queries);
}

void DBActionQueue::delItem( const afsql::DBItem * item)
{
   Queries * queries = new Queries();
   item->dbDelete( queries);
   push( queries);
}

void DBActionQueue::sendAlarm()
{
   std::string str("ALARM! Server database connection error. Contact your system administrator.");
   AFCommon::QueueLog( name + ":\n" + str);
   AfContainerLock mLock( m_monitors, AfContainerLock::WRITELOCK);
   m_monitors->sendMessage( str);
}

void DBActionQueue::sendConnected()
{
   std::string str("AFANASY: Server database connection established.");
   AFCommon::QueueLog( name + ":\n" + str);
   AfContainerLock mLock( m_monitors, AfContainerLock::WRITELOCK);
   m_monitors->sendMessage( str);
}
