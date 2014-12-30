/*
 ******************************************************************
 *           Delimiter Seperated Values Filter Library            *
 *                                                                *
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

#ifndef INCLUDE_DSV_FILTER_HPP
#define INCLUDE_DSV_FILTER_HPP


#include <string>
#include <deque>
#include <vector>

#define strtk_no_tr1_or_boost

#include "exprtk.hpp"
#include "strtk.hpp"


#ifdef dsv_filter_use_mmap
  #include <boost/iostreams/device/mapped_file.hpp>
#endif


class dsv_filter
{
public:

   struct column_properties
   {
      enum column_type
      {
         e_none   = 0,
         e_string = 1,
         e_number = 2
      };

      column_properties()
      : type(e_none),
        name   (""),
        value_s(""),
        value_n(0.0),
        process(false)
      {}

      column_type type;
      std::string name;
      std::string value_s;
      double value_n;
      strtk::util::value value;
      bool process;
   };

   dsv_filter()
   : file_name_       ("" ),
     input_delimiter_ (","),
     output_delimiter_("|")
   {
      symbol_table_.add_constants();
      expression_.register_symbol_table(symbol_table_);
   }

   inline std::string file_name() const
   {
      return file_name_;
   }

   inline void set_input_delimiter(const std::string& input_delimiter)
   {
      input_delimiter_ = input_delimiter;
   }

   inline void set_output_delimiter(const std::string& output_delimiter)
   {
      output_delimiter_ = output_delimiter;
   }

   inline std::string input_delimiter() const
   {
      return input_delimiter_;
   }

   inline std::string output_delimiter() const
   {
      return output_delimiter_;
   }

   inline std::size_t column_count() const
   {
      return column_.size();
   }

   inline std::size_t row_count() const
   {
      return grid_.row_count();
   }

   inline const column_properties& column(const std::size_t& index) const
   {
      return column_[index];
   }

   inline bool load(const std::string& file_name)
   {
      if (!strtk::fileio::file_exists(file_name))
         return false;

      file_name_ = file_name;
      strtk::token_grid::options options;
      options.column_delimiters = input_delimiter_;

      #ifdef dsv_filter_use_mmap
      input_source.close();
      input_source.open(file_name_);

      unsigned char* data = reinterpret_cast<unsigned char*>(const_cast<char*>(input_source.data()));

      if (!grid_.load(data,input_source.size(),options))
         return false;
      #else
         if (!grid_.load(file_name_,options))
            return false;
      #endif

      if (0 == grid_.row_count())
         return false;
      else if (grid_.row_count() < 2)
         return false;
      else if (!process_column_header())
         return false;

      return true;
   }

   inline bool add_filter(const std::string& filter_expression)
   {
      error_ = "";
      parser_.dec().collect_variables() = true;

      if (!parser_.compile(filter_expression,expression_))
      {
         error_ = "Error: " + parser_.error() + "\tFilter: " + filter_expression;

         return false;
      }

      // Only extract for processing, the column values
      // that are being used in the current expression.
      typedef exprtk::parser<double> parser_t;
      typedef parser_t::dependent_entity_collector::symbol_t symbol_t;

      std::deque<symbol_t> symbol_list;

      for (std::size_t i = 0; i < column_.size(); ++i)
      {
         if (column_[i].name.empty())
            continue;

         column_[i].process = false;

         for (std::size_t j = 0; j < symbol_list.size(); ++j)
         {
            if (strtk::imatch(symbol_list[j].first,column_[i].name))
            {
               column_[i].process = true;
               break;
            }
         }
      }

      return true;
   }

   template <typename Allocator,
             template <typename,typename> class Sequence>
   inline bool row(const std::size_t& r,
                   const Sequence<bool,Allocator>& selected_column,
                   std::string& row_result)
   {
      if (selected_column.size() != column_.size())
      {
         error_ = "Error: number of selected columns larger than number of columns";
         return false;
      }

      if (r >= grid_.row_count())
      {
         strtk::build_string s;
         s << "Error: row[" << r << "] out of bounds.";
         error_ = s.to_str();

         return false;
      }

      if (row_.index() != r)
      {
         row_ = grid_.row(r);
      }

      for (std::size_t c = 0; c < column_.size(); ++c)
      {
         if (selected_column[c])
         {
            strtk::token_grid::range_t token = row_.token(c);
            row_result.append(token.first,token.second);

            if (c < (column_.size() - 1))
            {
               row_result.append(output_delimiter_);
            }
         }
      }

      if (!row_result.empty() && row_result[row_result.size() - 1])
      {
         row_result.resize(row_result.size() - 1);
      }

      return true;
   }

   inline std::string error()
   {
      return error_;
   }

   enum filter_result
   {
      e_error,
      e_match,
      e_mismatch
   };

   inline filter_result operator[](const std::size_t& r)
   {
      row_ = grid_.row(r);

      for (std::size_t c = 0; c < column_.size(); ++c)
      {
         if (!column_[c].process)
            continue;
         else if (!row_.parse_with_index(c,column_[c].value))
         {
            strtk::build_string s;
            s << "Error: Failed to process element at row/col["<< r << "," << c << "]  value:" << row_.get<std::string>(c);
            error_ = s.to_str();

            return e_error;
         }
      }

      return (1.0 == expression_.value()) ? e_match : e_mismatch;
   }

   const strtk::token_grid& grid() const
   {
      return grid_;
   }

private:

   inline bool process_column_header()
   {
      static const std::string string_id ("_s");
      static const std::string number_id ("_n");

      expression_.get_symbol_table().clear();
      column_.clear();
      column_.resize(grid_.row(0).size());

      strtk::token_grid::row_type row = grid_.row(0);
      std::string col_name   = "";
      std::string col_suffix = "";

      for (std::size_t i = 0; i < row.size(); ++i)
      {
         column_properties& column = column_[i];

         column.process = false;

         col_name = row.get<std::string>(i);
         col_suffix = (col_name.size() >= 2) ? strtk::text::remaining_string(col_name.size() - 2,col_name) : "";
         col_name = col_name.substr(0,col_name.size() - 2);

         if (symbol_table_.symbol_exists(col_name))
         {
            error_ = "Error: Redefinition of column " + col_name;
            return false;
         }
         else if (strtk::iends_with("_s",col_suffix))
         {
            column.type    = dsv_filter::column_properties::e_string;
            column.name    = col_name;
            column.value   = strtk::util::value(column.value_s);
            column.process = true;

            symbol_table_.add_stringvar(col_name,column.value_s);
         }
         else if (strtk::iends_with("_n",col_suffix))
         {
            column.type    = dsv_filter::column_properties::e_number;
            column.name    = col_name;
            column.process = true;
            column.value   = strtk::util::value(column.value_n);

            symbol_table_.add_variable(col_name,column.value_n);
         }
      }

      return true;
   }

   std::string file_name_;
   std::string input_delimiter_;
   std::string output_delimiter_;
   std::string error_;
   std::vector<column_properties> column_;
   strtk::token_grid grid_;
   exprtk::symbol_table<double> symbol_table_;
   exprtk::parser<double> parser_;
   exprtk::expression<double> expression_;
   strtk::token_grid::row_type row_;

   #ifdef dsv_filter_use_mmap
   boost::iostreams::mapped_file_source input_source;
   #endif

};

#endif
