/*
 ******************************************************************
 *           Delimiter Seperated Values Filter Library            *
 *                                                                *
 * DSV Filter Example                                             *
 * Author: Arash Partow (2004)                                    *
 * URL: http://www.partow.net/programming/dsvfilter/index.html    *
 *                                                                *
 * Copyright notice:                                              *
 * Free use of the Delimiter Seperated Values Filter Library is   *
 * permitted under the guidelines and in accordance with the most *
 * current version of the Common Public License.                  *
 * http://www.opensource.org/licenses/cpl1.0.php                  *
 *                                                                *
 ******************************************************************
*/


/*
  Description:

  The following is a  simple driver demonstrating the  capabilities
  of the DSV  Filter library. The  example is capable  of loading a
  CSV or  DSV flat  text file  into memory,  and then  outputting a
  subset of rows based on a user specified filter expression.

  It is required that the first line of the input file be a  header
  line, which uniquely names each of the columns. If a column is to
  be used within expressions, then the name of the column must  end
  in either  an "_n"  or "_s"  denoting the  type of  the column as
  being either a number or string respectively.

  Furthermore  the  driver  supports  selectively  choosing   which
  columns to output upon successfully filtering  a row or simply to
  output the number of rows that have matched the specified  filter
  expression.

  Example Filter 1:

          col3 == (col2 + col6) or col5 > col1

  Explanation:
  For each row/tuple select for output where the value of col3 is
  equal to the sum of the values of col2 and col6 or if the value
  of col5 is greater than col1.


  Example Filter 2:

          select col1,col8 | col3 > col2 + col6

  Explanation:
  For each row, select the columns named col1 and col8 for output
  where the value for col3 is greater than the sum of the values
  of col2 and col6.


  Example Filter 3:

          count | col3 <= (col2 - col6) and 'Grape' in col7

  Explanation:
  Return the number of rows where the value for col3 is less than
  or equal to the difference of the values between col2 and col6
  and where the string 'Grape' appears in col7.

  It should be  noted that when  declaring an expression  or select
  list, that the column  names are not case  sensitive. Furthermore
  they should not include the "_s" or "_n" suffixes.

  The pipe symbol  "|"  indicates the  separation of the  select or
  count  clause  from  the where-clause  which  defines  the filter
  expression upon which every row will be evaluated against.


  Typical Usage:

  Step 1: Execute dsv_filter_example

  Step 2: Type 'help' at the prompt for a quick overview of the available commands

  Step 3: Set the desired input delimiter value by using the 'input_delimiter = <char>' command.
          By default it's pipe "|"

  Step 4: Load the data-store by using the 'load <file name>' command

  Step 5: Type 'information' to view details about the data-store

  Step 6: Type 'list' to view all the queryable columns

  Step 7: Enter query and press 'enter'

*/


#include <iostream>
#include <string>

#include "dsv_filter.hpp"
#include "strtk.hpp"


void display_columns(const dsv_filter& filter);

bool load_dsv(const std::string query, dsv_filter& filter);

template <typename Allocator,
          template <typename,typename> class Sequence>
void display_history(const Sequence<std::string,Allocator>& query_history);

template <typename Allocator,
          template <typename,typename> class Sequence>
void generate_results(const Sequence<bool,Allocator>& selected_column, dsv_filter& filter, const bool count_only = false);

template <typename Allocator,
          template <typename,typename> class Sequence>
bool parse_query(std::string& query, Sequence<bool,Allocator>& selected_column_list, bool& count_mode, dsv_filter& filter);

bool set_output_delimiter(const std::string& query, dsv_filter& filter);

bool set_input_delimiter(const std::string& query, dsv_filter& filter);

template <typename Allocator,
          template <typename,typename> class Sequence>
bool lookup_history(const Sequence<std::string,Allocator>& query_history, std::string& query);

void information(const dsv_filter& filter);

void print_help();

int main(int argc, char* argv[])
{
   //example usage:
   //dsv_filter_example <dsv file name>
   //dsv_filter_example <dsv file name> <input delimiter>
   //dsv_filter_example <dsv file name> <input delimiter> <output delimiter>

   std::string file_name = "";
   std::string input_delimiter = "|";
   std::string output_delimiter = "|";

   if (2 == argc)
   {
      file_name = argv[1];
   }
   else if (3 == argc)
   {
      file_name = argv[1];
      input_delimiter = argv[2];
   }
   else if (4 == argc)
   {
      file_name = argv[1];
      input_delimiter = argv[2];
      output_delimiter = argv[3];
   }

   dsv_filter filter;

   filter.set_input_delimiter(input_delimiter);
   filter.set_output_delimiter(output_delimiter);

   if (!file_name.empty())
   {
      if (filter.load(file_name))
         std::cout << "Successfully loaded " << filter.file_name() << "\n";
      else
      {
         std::cerr << "Error - Failed to load: " << file_name << std::endl;
         return 1;
      }
   }

   std::vector<bool> selected_column_list(filter.column_count(),true);

   std::deque<std::string> query_history;
   std::string query;

   for ( ; ; )
   {
      std::cout << "\nEnter query: ";
      std::getline(std::cin,query);

      strtk::remove_leading_trailing(" \t\n\r",query);

      if (query.empty())
         continue;
      else if (query_history.empty() || (query_history.back() != query))
      {
         query_history.push_back(query);
      }

      if (0 == strtk::ifind("exec",query))
      {
         if (!lookup_history(query_history,query))
            continue;
      }

      if (strtk::imatch(query,"exit") || strtk::imatch(query,"quit"))
         break;
      else if (strtk::imatch(query,"help"))
      {
         print_help();
         continue;
      }
      else if (strtk::imatch(query,"list"))
      {
         display_columns(filter);
         continue;
      }
      else if (0 == strtk::ifind("load",query))
      {
         load_dsv(query,filter);
         continue;
      }
      else if (strtk::imatch(query,"history"))
      {
         query_history.pop_back();
         display_history(query_history);
         continue;
      }
      else if (0 == strtk::ifind("output_delimiter",query))
      {
         set_output_delimiter(query,filter);
         continue;
      }
      else if (0 == strtk::ifind("input_delimiter",query))
      {
         set_input_delimiter(query,filter);
         continue;
      }
      else if ((0 == strtk::ifind("information",query)) || (0 == strtk::ifind("info",query)))
      {
         information(filter);
         continue;
      }

      selected_column_list.resize(filter.column_count(),true);
      std::fill_n(selected_column_list.begin(),selected_column_list.size(),true);

      bool count_mode = false;

      if(!parse_query(query,selected_column_list,count_mode,filter))
         continue;
      else if(filter.add_filter(query))
         generate_results(selected_column_list,filter,count_mode);
      else
      {
         std::cout << filter.error() << std::endl;
         continue;
      }
   }

   return 0;
}

void display_columns(const dsv_filter& filter)
{
   if (0 == filter.column_count())
      std::cout << "No valid columns available.\n";
   else
   {
      std::cout << "+--+----------------+---------+\n";
      std::cout << "|# |      Name      |   Type  |\n";
      std::cout << "+--+----------------+---------+\n";
      std::size_t length = 0;

      for (std::size_t i = 0; i < filter.column_count(); ++i)
      {
         const dsv_filter::column_properties& column = filter.column(i);
         length = std::max(length,column.name.size());
      }

      for (std::size_t i = 0; i < filter.column_count(); ++i)
      {
         const dsv_filter::column_properties& column = filter.column(i);
         std::cout << "|" << strtk::text::right_align(2,'0',i) << "| "
                   << strtk::text::left_align(length,column.name);
         switch (column.type)
         {
            case dsv_filter::column_properties::e_string : std::cout << " | STRING  |\n"; break;
            case dsv_filter::column_properties::e_number : std::cout << " | NUMBER  |\n"; break;
            default                                      : std::cout << " | UNKNOWN |\n"; break;
         }
      }

      std::cout << "+--+----------------+---------+\n";
   }
}

bool load_dsv(const std::string query, dsv_filter& filter)
{
   static strtk::ignore_token ignore;
   std::string file_name;

   if (!strtk::parse(query," \t",ignore,file_name))
      return true;

   strtk::util::timer timer;
   timer.start();

   if(!filter.load(file_name))
   {
      std::cout << "Failed to load: " << file_name << std::endl;
      return false;
   }

   timer.stop();

   printf("Successfully loaded: %s - Total time: %6.3fsec\n",
          file_name.c_str(),
          timer.time());

   return true;
}

template <typename Allocator,
          template <typename,typename> class Sequence>
void display_history(const Sequence<std::string,Allocator>& query_history)
{
   for (std::size_t i = 0; i < query_history.size(); ++i)
   {
      std::cout << strtk::text::right_align(2,'0',i) << " " << query_history[i] << std::endl;
   }

   std::cout << "Number of queries: " << query_history.size() << std::endl;
}


template <typename Allocator,
          template <typename,typename> class Sequence>
void generate_results(const Sequence<bool,Allocator>& selected_column, dsv_filter& filter, const bool count_only)
{
   strtk::util::timer timer;
   timer.start();

   std::size_t result_count = 0;
   std::size_t row_count = filter.row_count();
   dsv_filter::filter_result filter_result;

   std::string result;
   result.reserve(strtk::one_kilobyte);

   for (std::size_t r = 1; r < row_count; ++r)
   {
      filter_result = filter[r];

      if (dsv_filter::e_match == filter_result)
      {
         if (!count_only)
         {
            result.clear();

            if (!filter.row(r,selected_column,result))
            {
               std::cout << filter.error() << std::endl;
               break;
            }

            std::cout << result << "\n";
         }

         ++result_count;
      }
      else if (dsv_filter::e_error == filter_result)
      {
         std::cout << filter.error() << std::endl;
         break;
      }
   }

   timer.stop();

   std::cout << "---------------------\n";
   printf("Number of results: %d\nTime: %8.5fms\n",
          static_cast<unsigned int>(result_count),
          timer.time() * 1000.0);
}

template <typename Allocator,
          template <typename,typename> class Sequence>
bool parse_query(std::string& query,
                 Sequence<bool,Allocator>& selected_column_list,
                 bool& count_mode,
                 dsv_filter& filter)
{
   std::deque<std::string> sub_query;
   strtk::parse(query,"|",sub_query);

   strtk::remove_empty_strings(sub_query);

   if (2 == sub_query.size())
   {
      if (0 == strtk::ifind("select",sub_query[0]))
      {
         std::fill_n(selected_column_list.begin(),selected_column_list.size(),false);

         std::deque<std::string> selected_cols;

         strtk::parse(sub_query[0],", ",selected_cols);
         bool col_found = false;

         for (std::size_t i = 1; i < selected_cols.size(); ++i)
         {
            if (selected_cols[i].empty())
               continue;

            col_found = false;

            for (std::size_t c = 0; c < filter.column_count(); ++c)
            {
               if (strtk::imatch(selected_cols[i],filter.column(c).name))
               {
                  selected_column_list[c] = true;
                  col_found = true;
                  break;
               }
            }

            if (!col_found)
            {
               std::cout << "Error - Invalid column: [" << selected_cols[i] << "]" << std::endl;
               return false;
            }
         }

         query = sub_query[1];
      }
      else if (0 == strtk::ifind("count",sub_query[0]))
      {
         count_mode = true;
         query = sub_query[1];
      }
      else
         return false;
   }
   else if (1 != sub_query.size())
      return false;
   else
      query = sub_query[0];

   return true;
}

bool set_output_delimiter(const std::string& query, dsv_filter& filter)
{
   static const std::string preamble = "output_delimiter = ";

   if (0 != strtk::ifind("output_delimiter = ",query))
   {
      std::cout << "Invalid format for command. eg: output_delimiter = |" << std::endl;
      return false;
   }

   std::string delimiter = strtk::text::remaining_string(preamble.size(),query);
   strtk::remove_leading_trailing(" '\"",delimiter);

   if (delimiter.empty())
      return false;

   filter.set_output_delimiter(delimiter);
   std::cout << "Output delimiter set to: " << delimiter << std::endl;

   return true;
}

bool set_input_delimiter(const std::string& query, dsv_filter& filter)
{
   static const std::string preamble = "input_delimiter = ";

   if (0 != strtk::ifind("input_delimiter = ",query))
   {
      std::cout << "Invalid format for command. eg: input_delimiter = |" << std::endl;
      return false;
   }

   std::string delimiter = strtk::text::remaining_string(preamble.size(),query);
   strtk::remove_leading_trailing(" '\"",delimiter);

   if (delimiter.empty())
      return false;

   filter.set_input_delimiter(delimiter);
   std::cout << "Input delimiter set to: " << delimiter << std::endl;

   return true;
}

template <typename Allocator,
          template <typename,typename> class Sequence>
bool lookup_history(const Sequence<std::string,Allocator>& query_history, std::string& query)
{
   if (query_history.empty())
      return false;

   static strtk::ignore_token ignore;
   std::size_t query_index = 0;

   if (!strtk::parse(query," \t",ignore,query_index))
      return false;
   else if (query_index >= query_history.size())
      return false;

   query = query_history[query_index];
   std::cout << "Query: " << query << std::endl;

   return true;
}

void information(const dsv_filter& filter)
{
   std::cout << "\n";
   std::cout << "Information\n";
   std::cout << "-----------\n";
   std::cout << "File:             " << filter.file_name()    << "\n";
   std::cout << "Rows:             " << filter.row_count()    << "\n";
   std::cout << "Columns:          " << filter.column_count() << "\n";
   std::cout << "Elements:         " << filter.row_count() * filter.column_count() << "\n";
   std::cout << "Input Delimiter:  " << filter.input_delimiter()  << "\n";
   std::cout << "Output Delimiter: " << filter.output_delimiter() << "\n";
   std::cout << "\n";
}

void print_help()
{
   std::cout << "\n";
   std::cout << "DSV Filter Help\n";
   std::cout << "Command                   Definition\n";
   std::cout << "------------------------------------\n";
   std::cout << "list                      List columns and their types\n";
   std::cout << "history                   Display history of queries\n";
   std::cout << "load <file>               Load DSV file\n";
   std::cout << "exec <i'th query>         Execute the i'th query found in history\n";
   std::cout << "input_delimiter = <char>  Set the input delimiter\n";
   std::cout << "output_delimiter = <char> Set the output delimiter\n";
   std::cout << "information               Display details of DSV store\n";
   std::cout << "\n";
}
