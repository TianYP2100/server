#ifndef PARTITION_INFO_INCLUDED
#define PARTITION_INFO_INCLUDED

/* Copyright (c) 2006, 2010, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1335  USA */

#ifdef USE_PRAGMA_INTERFACE
#pragma interface			/* gcc class implementation */
#endif

#include "sql_class.h"
#include "partition_element.h"
#include "sql_partition.h"

class partition_info;
struct TABLE_LIST;
/* Some function typedefs */
typedef int (*get_part_id_func)(partition_info *part_info, uint32 *part_id,
                                 longlong *func_value);
typedef int (*get_subpart_id_func)(partition_info *part_info, uint32 *part_id);
typedef bool (*check_constants_func)(THD *thd, partition_info *part_info);
 
struct st_ddl_log_memory_entry;

struct Vers_part_info : public Sql_alloc
{
  Vers_part_info() :
    limit(0),
    now_part(NULL),
    hist_part(NULL)
  {
    interval.type= INTERVAL_LAST;
  }
  Vers_part_info(Vers_part_info &src) :
    interval(src.interval),
    limit(src.limit),
    now_part(NULL),
    hist_part(NULL)
  {
  }
  bool initialized()
  {
    if (now_part)
    {
      DBUG_ASSERT(now_part->id != UINT_MAX32);
      DBUG_ASSERT(now_part->type == partition_element::CURRENT);
      if (hist_part)
      {
        DBUG_ASSERT(hist_part->id != UINT_MAX32);
        DBUG_ASSERT(hist_part->type == partition_element::HISTORY);
      }
      return true;
    }
    return false;
  }
  struct {
    my_time_t start;
    INTERVAL step;
    enum interval_type type;
    bool is_set() { return type < INTERVAL_LAST; }
  } interval;
  ulonglong limit;
  partition_element *now_part;
  partition_element *hist_part;
};

class partition_info : public Sql_alloc
{
public:
  /*
   * Here comes a set of definitions needed for partitioned table handlers.
   */
  List<partition_element> partitions;
  List<partition_element> temp_partitions;

  List<const char> part_field_list;
  List<const char> subpart_field_list;
  
  /* 
    If there is no subpartitioning, use only this func to get partition ids.
    If there is subpartitioning, use the this func to get partition id when
    you have both partition and subpartition fields.
  */
  get_part_id_func get_partition_id;

  /* Get partition id when we don't have subpartition fields */
  get_part_id_func get_part_partition_id;

  /* 
    Get subpartition id when we have don't have partition fields by we do
    have subpartition ids.
    Mikael said that for given constant tuple 
    {subpart_field1, ..., subpart_fieldN} the subpartition id will be the
    same in all subpartitions
  */
  get_subpart_id_func get_subpartition_id;

  /*
    When we have various string fields we might need some preparation
    before and clean-up after calling the get_part_id_func's. We need
    one such method for get_part_partition_id and one for
    get_subpartition_id.
  */
  get_part_id_func get_part_partition_id_charset;
  get_subpart_id_func get_subpartition_id_charset;

  check_constants_func check_constants;

  /* NULL-terminated array of fields used in partitioned expression */
  Field **part_field_array;
  Field **subpart_field_array;
  Field **part_charset_field_array;
  Field **subpart_charset_field_array;
  /* 
    Array of all fields used in partition and subpartition expression,
    without duplicates, NULL-terminated.
  */
  Field **full_part_field_array;
  /*
    Set of all fields used in partition and subpartition expression.
    Required for testing of partition fields in write_set when
    updating. We need to set all bits in read_set because the row may
    need to be inserted in a different [sub]partition.
  */
  MY_BITMAP full_part_field_set;

  /*
    When we have a field that requires transformation before calling the
    partition functions we must allocate field buffers for the field of
    the fields in the partition function.
  */
  uchar **part_field_buffers;
  uchar **subpart_field_buffers;
  uchar **restore_part_field_ptrs;
  uchar **restore_subpart_field_ptrs;

  Item *part_expr;
  Item *subpart_expr;

  Item *item_free_list;

  struct st_ddl_log_memory_entry *first_log_entry;
  struct st_ddl_log_memory_entry *exec_log_entry;
  struct st_ddl_log_memory_entry *frm_log_entry;

  /* 
    Bitmaps of partitions used by the current query. 
    * read_partitions  - partitions to be used for reading.
    * lock_partitions  - partitions that must be locked (read or write).
    Usually read_partitions is the same set as lock_partitions, but
    in case of UPDATE the WHERE clause can limit the read_partitions set,
    but not neccesarily the lock_partitions set.
    Usage pattern:
    * Initialized in ha_partition::open().
    * read+lock_partitions is set  according to explicit PARTITION,
      WL#5217, in open_and_lock_tables().
    * Bits in read_partitions can be cleared in prune_partitions()
      in the optimizing step.
      (WL#4443 is about allowing prune_partitions() to affect lock_partitions
      and be done before locking too).
    * When the partition enabled handler get an external_lock call it locks
      all partitions in lock_partitions (and remembers which partitions it
      locked, so that it can unlock them later). In case of LOCK TABLES it will
      lock all partitions, and keep them locked while lock_partitions can
      change for each statement under LOCK TABLES.
    * Freed at the same time item_free_list is freed.
  */
  MY_BITMAP read_partitions;
  MY_BITMAP lock_partitions;
  bool bitmaps_are_initialized;

  union {
    longlong *range_int_array;
    LIST_PART_ENTRY *list_array;
    part_column_list_val *range_col_array;
    part_column_list_val *list_col_array;
  };

  Vers_part_info *vers_info;
  
  /********************************************
   * INTERVAL ANALYSIS
   ********************************************/
  /*
    Partitioning interval analysis function for partitioning, or NULL if 
    interval analysis is not supported for this kind of partitioning.
  */
  get_partitions_in_range_iter get_part_iter_for_interval;
  /*
    Partitioning interval analysis function for subpartitioning, or NULL if
    interval analysis is not supported for this kind of partitioning.
  */
  get_partitions_in_range_iter get_subpart_iter_for_interval;
  
  /********************************************
   * INTERVAL ANALYSIS ENDS 
   ********************************************/

  longlong err_value;
  char* part_info_string;

  partition_element *curr_part_elem;     // part or sub part
  partition_element *current_partition;  // partition
  part_elem_value *curr_list_val;
  uint curr_list_object;
  uint num_columns;

  TABLE *table;
  /*
    These key_map's are used for Partitioning to enable quick decisions
    on whether we can derive more information about which partition to
    scan just by looking at what index is used.
  */
  key_map all_fields_in_PF, all_fields_in_PPF, all_fields_in_SPF;
  key_map some_fields_in_PF;

  handlerton *default_engine_type;
  partition_type part_type;
  partition_type subpart_type;

  uint part_info_len;

  uint num_parts;
  uint num_subparts;
  uint count_curr_subparts;                  // used during parsing

  uint num_list_values;

  uint num_part_fields;
  uint num_subpart_fields;
  uint num_full_part_fields;

  uint has_null_part_id;
  uint32 default_partition_id;
  /*
    This variable is used to calculate the partition id when using
    LINEAR KEY/HASH. This functionality is kept in the MySQL Server
    but mainly of use to handlers supporting partitioning.
  */
  uint16 linear_hash_mask;
  /*
    PARTITION BY KEY ALGORITHM=N
    Which algorithm to use for hashing the fields.
    N = 1 - Use 5.1 hashing (numeric fields are hashed as binary)
    N = 2 - Use 5.5 hashing (numeric fields are hashed like latin1 bytes)
  */
  enum enum_key_algorithm
    {
      KEY_ALGORITHM_NONE= 0,
      KEY_ALGORITHM_51= 1,
      KEY_ALGORITHM_55= 2
    };
  enum_key_algorithm key_algorithm;

  /* Only the number of partitions defined (uses default names and options). */
  bool use_default_partitions;
  bool use_default_num_partitions;
  /* Only the number of subpartitions defined (uses default names etc.). */
  bool use_default_subpartitions;
  bool use_default_num_subpartitions;
  bool default_partitions_setup;
  bool defined_max_value;
  inline bool has_default_partititon()
  {
    return (part_type == LIST_PARTITION && defined_max_value);
  }
  bool list_of_part_fields;                  // KEY or COLUMNS PARTITIONING
  bool list_of_subpart_fields;               // KEY SUBPARTITIONING
  bool linear_hash_ind;                      // LINEAR HASH/KEY
  bool fixed;
  bool is_auto_partitioned;
  bool has_null_value;
  bool column_list;                          // COLUMNS PARTITIONING, 5.5+

  partition_info()
  : get_partition_id(NULL), get_part_partition_id(NULL),
    get_subpartition_id(NULL),
    part_field_array(NULL), subpart_field_array(NULL),
    part_charset_field_array(NULL),
    subpart_charset_field_array(NULL),
    full_part_field_array(NULL),
    part_field_buffers(NULL), subpart_field_buffers(NULL),
    restore_part_field_ptrs(NULL), restore_subpart_field_ptrs(NULL),
    part_expr(NULL), subpart_expr(NULL), item_free_list(NULL),
    first_log_entry(NULL), exec_log_entry(NULL), frm_log_entry(NULL),
    bitmaps_are_initialized(FALSE),
    list_array(NULL), vers_info(NULL), err_value(0),
    part_info_string(NULL),
    curr_part_elem(NULL), current_partition(NULL),
    curr_list_object(0), num_columns(0), table(NULL),
    default_engine_type(NULL),
    part_type(NOT_A_PARTITION), subpart_type(NOT_A_PARTITION),
    part_info_len(0),
    num_parts(0), num_subparts(0),
    count_curr_subparts(0),
    num_list_values(0), num_part_fields(0), num_subpart_fields(0),
    num_full_part_fields(0), has_null_part_id(0), linear_hash_mask(0),
    key_algorithm(KEY_ALGORITHM_NONE),
    use_default_partitions(TRUE), use_default_num_partitions(TRUE),
    use_default_subpartitions(TRUE), use_default_num_subpartitions(TRUE),
    default_partitions_setup(FALSE), defined_max_value(FALSE),
    list_of_part_fields(FALSE), list_of_subpart_fields(FALSE),
    linear_hash_ind(FALSE), fixed(FALSE),
    is_auto_partitioned(FALSE),
    has_null_value(FALSE), column_list(FALSE)
  {
    all_fields_in_PF.clear_all();
    all_fields_in_PPF.clear_all();
    all_fields_in_SPF.clear_all();
    some_fields_in_PF.clear_all();
    partitions.empty();
    temp_partitions.empty();
    part_field_list.empty();
    subpart_field_list.empty();
  }
  ~partition_info() {}

  partition_info *get_clone(THD *thd, bool empty_data_and_index_file= FALSE);
  bool set_named_partition_bitmap(const char *part_name, size_t length);
  bool set_partition_bitmaps(List<String> *partition_names);
  bool set_partition_bitmaps_from_table(TABLE_LIST *table_list);
  /* Answers the question if subpartitioning is used for a certain table */
  bool is_sub_partitioned()
  {
    return (subpart_type == NOT_A_PARTITION ?  FALSE : TRUE);
  }

  /* Returns the total number of partitions on the leaf level */
  uint get_tot_partitions()
  {
    return num_parts * (is_sub_partitioned() ? num_subparts : 1);
  }

  bool set_up_defaults_for_partitioning(THD *thd, handler *file,
                                        HA_CREATE_INFO *info,
                                        uint start_no);
  const char *find_duplicate_field();
  char *find_duplicate_name();
  bool check_engine_mix(handlerton *engine_type, bool default_engine);
  bool check_partition_info(THD *thd, handlerton **eng_type,
                            handler *file, HA_CREATE_INFO *info,
                            partition_info *add_or_reorg_part= NULL);
  void print_no_partition_found(TABLE *table, myf errflag);
  void print_debug(const char *str, uint*);
  Item* get_column_item(Item *item, Field *field);
  int fix_partition_values(THD *thd,
                           part_elem_value *val,
                           partition_element *part_elem);
  bool fix_column_value_functions(THD *thd,
                                  part_elem_value *val,
                                  uint part_id);
  bool fix_parser_data(THD *thd);
  int add_max_value(THD *thd);
  void init_col_val(part_column_list_val *col_val, Item *item);
  int reorganize_into_single_field_col_val(THD *thd);
  part_column_list_val *add_column_value(THD *thd);
  bool set_part_expr(THD *thd, Item *item_ptr, bool is_subpart);
  bool set_up_charset_field_preps(THD *thd);
  bool check_partition_field_length();
  bool init_column_part(THD *thd);
  bool add_column_list_value(THD *thd, Item *item);
  partition_element *get_part_elem(const char *partition_name, char *file_name,
                                   size_t file_name_size, uint32 *part_id);
  void report_part_expr_error(bool use_subpart_expr);
  bool has_same_partitioning(partition_info *new_part_info);
  bool error_if_requires_values() const;
private:
  bool set_up_default_partitions(THD *thd, handler *file, HA_CREATE_INFO *info,
                                 uint start_no);
  bool set_up_default_subpartitions(THD *thd, handler *file,
                                    HA_CREATE_INFO *info);
  char *create_default_partition_names(THD *thd, uint part_no, uint num_parts,
                                       uint start_no);
  char *create_default_subpartition_name(THD *thd, uint subpart_no,
                                         const char *part_name);
  // FIXME: prune_partition_bitmaps() is duplicate of set_read_partitions()
  bool prune_partition_bitmaps(List<String> *partition_names);
  bool add_named_partition(const char *part_name, size_t length);
public:
  bool set_read_partitions(List<char> *partition_names);
  bool has_unique_name(partition_element *element);
  bool field_in_partition_expr(Field *field) const;

  bool vers_init_info(THD *thd);
  bool vers_set_interval(THD *thd, Item *item,
                         interval_type int_type, my_time_t start)
  {
    DBUG_ASSERT(part_type == VERSIONING_PARTITION);
    vers_info->interval.type= int_type;
    vers_info->interval.start= start;
    if (item->fix_fields_if_needed_for_scalar(thd, &item))
      return true;
    bool error= get_interval_value(item, int_type, &vers_info->interval.step) ||
           vers_info->interval.step.neg || vers_info->interval.step.second_part ||
          !(vers_info->interval.step.year || vers_info->interval.step.month ||
            vers_info->interval.step.day || vers_info->interval.step.hour ||
            vers_info->interval.step.minute || vers_info->interval.step.second);
    if (error)
    {
      my_error(ER_PART_WRONG_VALUE, MYF(0),
               thd->lex->create_last_non_select_table->table_name.str,
               "INTERVAL");
    }
    return error;
  }
  bool vers_set_limit(ulonglong limit)
  {
    DBUG_ASSERT(part_type == VERSIONING_PARTITION);
    vers_info->limit= limit;
    return !limit;
  }
  int vers_set_hist_part(THD *thd);
  bool vers_setup_expression(THD *thd, uint32 alter_add= 0); /* Stage 1. */
  partition_element *get_partition(uint part_id)
  {
    List_iterator<partition_element> it(partitions);
    partition_element *el;
    while ((el= it++))
    {
      if (el->id == part_id)
        return el;
    }
    return NULL;
  }
};

uint32 get_next_partition_id_range(struct st_partition_iter* part_iter);
bool check_partition_dirs(partition_info *part_info);

/* Initialize the iterator to return a single partition with given part_id */

static inline void init_single_partition_iterator(uint32 part_id,
                                           PARTITION_ITERATOR *part_iter)
{
  part_iter->part_nums.start= part_iter->part_nums.cur= part_id;
  part_iter->part_nums.end= part_id+1;
  part_iter->ret_null_part= part_iter->ret_null_part_orig= FALSE;
  part_iter->ret_default_part= part_iter->ret_default_part_orig= FALSE;
  part_iter->get_next= get_next_partition_id_range;
}

/* Initialize the iterator to enumerate all partitions */
static inline
void init_all_partitions_iterator(partition_info *part_info,
                                  PARTITION_ITERATOR *part_iter)
{
  part_iter->part_nums.start= part_iter->part_nums.cur= 0;
  part_iter->part_nums.end= part_info->num_parts;
  part_iter->ret_null_part= part_iter->ret_null_part_orig= FALSE;
  part_iter->ret_default_part= part_iter->ret_default_part_orig= FALSE;
  part_iter->get_next= get_next_partition_id_range;
}

#endif /* PARTITION_INFO_INCLUDED */
