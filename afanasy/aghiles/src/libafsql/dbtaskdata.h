#pragma once

#include "../libafanasy/taskdata.h"

#include "dbitem.h"

namespace afsql
{
class DBTaskData: public DBItem, public af::TaskData
{
public:
   DBTaskData();
   DBTaskData( af::Msg * msg);
   virtual ~DBTaskData();

   inline const std::string & dbGetTableName() const { return TableName;}
   static const std::string dbWhereSelect( int id_job, int id_block, int id_task);

   static bool dbPrepareInsert( PGconn * i_conn);
   bool dbPrepareInsertExec( int id_job, int id_block, int id_task, PGconn * i_conn) const;

protected:
   inline const std::string & dbGetIDsString()  const { return IDs;      }
   inline const std::string & dbGetKeysString() const { return Keys;     }

protected:
   virtual void readwrite( af::Msg * msg);

private:
   static const std::string TableName;
   static const std::string Keys;
   static const std::string IDs;

   static const char ms_db_prepare_name[];

private:
   void addDBAttributes();
};
}
