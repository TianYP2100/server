/* Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2.0,
  as published by the Free Software Foundation.

  This program is also distributed with certain software (including
  but not limited to OpenSSL) that is licensed under separate terms,
  as designated in a particular file or component or in included license
  documentation.  The authors of MySQL hereby grant you an additional
  permission to link the program and your derivative works with the
  separately licensed software that they have included with MySQL.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License, version 2.0, for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1335 USA */

/**
  @file storage/perfschema/table_setup_objects.cc
  Table SETUP_OBJECTS (implementation).
*/

#include "my_global.h"
#include "my_pthread.h"
#include "pfs_instr.h"
#include "pfs_column_types.h"
#include "pfs_column_values.h"
#include "pfs_setup_object.h"
#include "table_setup_objects.h"
#include "table_helper.h"
#include "pfs_global.h"

THR_LOCK table_setup_objects::m_table_lock;

PFS_engine_table_share
table_setup_objects::m_share=
{
  { C_STRING_WITH_LEN("setup_objects") },
  &pfs_editable_acl,
  table_setup_objects::create,
  table_setup_objects::write_row,
  table_setup_objects::delete_all_rows,
  table_setup_objects::get_row_count,
  1000, /* records */
  sizeof(PFS_simple_index),
  &m_table_lock,
  { C_STRING_WITH_LEN("CREATE TABLE setup_objects("
                      "OBJECT_TYPE ENUM ('TABLE') not null default 'TABLE' comment 'Type of object to instrument. Currently, only TABLE, for base table.',"
                      "OBJECT_SCHEMA VARCHAR(64) default '%' comment 'Schema containing the object, either the literal or % for any schema.',"
                      "OBJECT_NAME VARCHAR(64) not null default '%' comment 'Name of the instrumented object, either the literal or % for any object.',"
                      "ENABLED ENUM ('YES', 'NO') not null default 'YES' comment 'Whether the object''s events are instrumented or not. Can be disabled, in which case monitoring is not enabled for those objects.',"
                      "TIMED ENUM ('YES', 'NO') not null default 'YES' comment 'Whether the object''s events are timed or not. Can be modified.')") }
};

int update_derived_flags()
{
  PFS_thread *thread= PFS_thread::get_current_thread();
  if (unlikely(thread == NULL))
    return HA_ERR_OUT_OF_MEM;

  update_table_share_derived_flags(thread);
  update_table_derived_flags();
  return 0;
}

PFS_engine_table* table_setup_objects::create(void)
{
  return new table_setup_objects();
}

int table_setup_objects::write_row(TABLE *table, unsigned char *buf,
                                   Field **fields)
{
  int result;
  Field *f;
  enum_object_type object_type= OBJECT_TYPE_TABLE;
  String object_schema_data("%", 1, &my_charset_utf8_bin);
  String object_name_data("%", 1, &my_charset_utf8_bin);
  String *object_schema= &object_schema_data;
  String *object_name= &object_name_data;
  enum_yes_no enabled_value= ENUM_YES;
  enum_yes_no timed_value= ENUM_YES;
  bool enabled= true;
  bool timed= true;

  for (; (f= *fields) ; fields++)
  {
    if (bitmap_is_set(table->write_set, f->field_index))
    {
      switch(f->field_index)
      {
      case 0: /* OBJECT_TYPE */
        object_type= (enum_object_type) get_field_enum(f);
        break;
      case 1: /* OBJECT_SCHEMA */
        object_schema= get_field_varchar_utf8(f, &object_schema_data);
        break;
      case 2: /* OBJECT_NAME */
        object_name= get_field_varchar_utf8(f, &object_name_data);
        break;
      case 3: /* ENABLED */
        enabled_value= (enum_yes_no) get_field_enum(f);
        break;
      case 4: /* TIMED */
        timed_value= (enum_yes_no) get_field_enum(f);
        break;
      default:
        DBUG_ASSERT(false);
      }
    }
  }

  /* Reject illegal enum values in OBJECT_TYPE */
  if (object_type != OBJECT_TYPE_TABLE)
    return HA_ERR_NO_REFERENCED_ROW;

  /* Reject illegal enum values in ENABLED */
  if ((enabled_value != ENUM_YES) && (enabled_value != ENUM_NO))
    return HA_ERR_NO_REFERENCED_ROW;

  /* Reject illegal enum values in TIMED */
  if ((timed_value != ENUM_YES) && (timed_value != ENUM_NO))
    return HA_ERR_NO_REFERENCED_ROW;

  enabled= (enabled_value == ENUM_YES) ? true : false;
  timed= (timed_value == ENUM_YES) ? true : false;

  result= insert_setup_object(object_type, object_schema, object_name,
                              enabled, timed);
  if (result == 0)
    result= update_derived_flags();
  return result;
}

int table_setup_objects::delete_all_rows(void)
{
  int result= reset_setup_object();
  if (result == 0)
    result= update_derived_flags();
  return result;
}

ha_rows table_setup_objects::get_row_count(void)
{
  return setup_object_count();
}

table_setup_objects::table_setup_objects()
  : PFS_engine_table(&m_share, &m_pos),
  m_row_exists(false), m_pos(0), m_next_pos(0)
{}

void table_setup_objects::reset_position(void)
{
  m_pos.m_index= 0;
  m_next_pos.m_index= 0;
}

int table_setup_objects::rnd_next(void)
{
  PFS_setup_object *pfs;

  for (m_pos.set_at(&m_next_pos);
       m_pos.m_index < setup_object_max;
       m_pos.next())
  {
    pfs= &setup_object_array[m_pos.m_index];
    if (pfs->m_lock.is_populated())
    {
      make_row(pfs);
      m_next_pos.set_after(&m_pos);
      return 0;
    }
  }

  return HA_ERR_END_OF_FILE;
}

int table_setup_objects::rnd_pos(const void *pos)
{
  PFS_setup_object *pfs;

  set_position(pos);

  DBUG_ASSERT(m_pos.m_index < setup_object_max);
  pfs= &setup_object_array[m_pos.m_index];
  if (pfs->m_lock.is_populated())
  {
    make_row(pfs);
    return 0;
  }

  return HA_ERR_RECORD_DELETED;
}

void table_setup_objects::make_row(PFS_setup_object *pfs)
{
  pfs_lock lock;

  m_row_exists= false;

  pfs->m_lock.begin_optimistic_lock(&lock);

  m_row.m_object_type= pfs->get_object_type();
  memcpy(m_row.m_schema_name, pfs->m_schema_name, pfs->m_schema_name_length);
  m_row.m_schema_name_length= pfs->m_schema_name_length;
  memcpy(m_row.m_object_name, pfs->m_object_name, pfs->m_object_name_length);
  m_row.m_object_name_length= pfs->m_object_name_length;
  m_row.m_enabled_ptr= &pfs->m_enabled;
  m_row.m_timed_ptr= &pfs->m_timed;

  if (pfs->m_lock.end_optimistic_lock(&lock))
    m_row_exists= true;
}

int table_setup_objects::read_row_values(TABLE *table,
                                         unsigned char *buf,
                                         Field **fields,
                                         bool read_all)
{
  Field *f;

  if (unlikely(! m_row_exists))
    return HA_ERR_RECORD_DELETED;

  /* Set the null bits */
  DBUG_ASSERT(table->s->null_bytes == 1);
  buf[0]= 0;

  for (; (f= *fields) ; fields++)
  {
    if (read_all || bitmap_is_set(table->read_set, f->field_index))
    {
      switch(f->field_index)
      {
      case 0: /* OBJECT_TYPE */
        set_field_enum(f, m_row.m_object_type);
        break;
      case 1: /* OBJECT_SCHEMA */
        if (m_row.m_schema_name_length)
          set_field_varchar_utf8(f, m_row.m_schema_name,
                                 m_row.m_schema_name_length);
        else
          f->set_null();
        break;
      case 2: /* OBJECT_NAME */
        if (m_row.m_object_name_length)
          set_field_varchar_utf8(f, m_row.m_object_name,
                                 m_row.m_object_name_length);
        else
          f->set_null();
        break;
      case 3: /* ENABLED */
        set_field_enum(f, (*m_row.m_enabled_ptr) ? ENUM_YES : ENUM_NO);
        break;
      case 4: /* TIMED */
        set_field_enum(f, (*m_row.m_timed_ptr) ? ENUM_YES : ENUM_NO);
        break;
      default:
        DBUG_ASSERT(false);
      }
    }
  }

  return 0;
}

int table_setup_objects::update_row_values(TABLE *table,
                                           const unsigned char *,
                                           const unsigned char *,
                                           Field **fields)
{
  int result;
  Field *f;
  enum_yes_no value;

  for (; (f= *fields) ; fields++)
  {
    if (bitmap_is_set(table->write_set, f->field_index))
    {
      switch(f->field_index)
      {
      case 0: /* OBJECT_TYPE */
      case 1: /* OBJECT_SCHEMA */
      case 2: /* OBJECT_NAME */
        return HA_ERR_WRONG_COMMAND;
      case 3: /* ENABLED */
        value= (enum_yes_no) get_field_enum(f);
        /* Reject illegal enum values in ENABLED */
        if ((value != ENUM_YES) && (value != ENUM_NO))
          return HA_ERR_NO_REFERENCED_ROW;
        *m_row.m_enabled_ptr= (value == ENUM_YES) ? true : false;
        break;
      case 4: /* TIMED */
        value= (enum_yes_no) get_field_enum(f);
        /* Reject illegal enum values in TIMED */
        if ((value != ENUM_YES) && (value != ENUM_NO))
          return HA_ERR_NO_REFERENCED_ROW;
        *m_row.m_timed_ptr= (value == ENUM_YES) ? true : false;
        break;
      default:
        DBUG_ASSERT(false);
      }
    }
  }

  result= update_derived_flags();
  return result;
}

int table_setup_objects::delete_row_values(TABLE *table,
                                           const unsigned char *buf,
                                           Field **fields)
{
  DBUG_ASSERT(m_row_exists);

  CHARSET_INFO *cs= &my_charset_utf8_bin;
  enum_object_type object_type= OBJECT_TYPE_TABLE;
  String object_schema(m_row.m_schema_name, m_row.m_schema_name_length, cs);
  String object_name(m_row.m_object_name, m_row.m_object_name_length, cs);

  int result= delete_setup_object(object_type, &object_schema, &object_name);

  if (result == 0)
    result= update_derived_flags();
  return result;
}

