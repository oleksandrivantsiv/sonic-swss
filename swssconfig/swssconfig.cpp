#include <stdlib.h>
#include <iostream>
#include <vector>
#include "logger.h"
#include <fstream>
#include "dbconnector.h"
#include "producertable.h"
#include "json.hpp"

using namespace std;
using namespace swss;
using json = nlohmann::json;

int db_port                          = 6379;
const char* const hostname           = "localhost";
const char* const op_name            = "OP";
const char* const name_delimiter     = ":";
const int el_count = 2;

void usage(char **argv)
{
    cout << "Usage: " << argv[0] << " json_file_path\n";
}

void dump_db_item(KeyOpFieldsValuesTuple &db_item)
{
    SWSS_LOG_NOTICE("db_item: [");
    SWSS_LOG_NOTICE("\toperation: %s", kfvOp(db_item).c_str());
    SWSS_LOG_NOTICE("\thash: %s", kfvKey(db_item).c_str());
    SWSS_LOG_NOTICE("\tfields: [");
    for (auto fv: kfvFieldsValues(db_item))
        SWSS_LOG_NOTICE("\t\tfield: %s value: %s", fvField(fv).c_str(), fvValue(fv).c_str());
    SWSS_LOG_NOTICE("\t]");
    SWSS_LOG_NOTICE("]");
}

bool write_db_data(vector<KeyOpFieldsValuesTuple> &db_items)
{
    DBConnector db(APPL_DB, hostname, db_port, 0);
    for (auto &db_item : db_items)
    {
        dump_db_item(db_item);

        string key = kfvKey(db_item);
        size_t pos = key.find(name_delimiter);
        if ((string::npos == pos) || ((key.size() - 1) == pos))
        {
            SWSS_LOG_ERROR("Invalid formatted hash:%s\n", key.c_str());
            return false;
        }
        string table_name = key.substr(0, pos);
        string key_name = key.substr(pos + 1);
        ProducerTable producer(&db, table_name);

        if (kfvOp(db_item) == SET_COMMAND)
            producer.set(key_name, kfvFieldsValues(db_item), SET_COMMAND);
        if (kfvOp(db_item) == DEL_COMMAND)
            producer.del(key_name, DEL_COMMAND);
    }
    return true;
}

bool load_json_db_data(iostream &fs, vector<KeyOpFieldsValuesTuple> &db_items)
{
    json json_array;
    fs >> json_array;

    if (!json_array.is_array())
    {
        SWSS_LOG_ERROR("Root element must be an array.");
        return false;
    }

    for (size_t i = 0; i < json_array.size(); i++)
    {
        auto &arr_item = json_array[i];

        if (arr_item.is_object())
        {
            if (el_count != arr_item.size())
            {
                SWSS_LOG_ERROR("Chlid elements must have both key and op entry. %s",
                               arr_item.dump().c_str());
                return false;
            }

            db_items.push_back(KeyOpFieldsValuesTuple());
            auto &cur_db_item = db_items[db_items.size() - 1];

            for (json::iterator child_it = arr_item.begin(); child_it != arr_item.end(); child_it++) {
                auto cur_obj_key = child_it.key();
                auto &cur_obj = child_it.value();

                string field_str;
                string value_str;

                if (cur_obj.is_object()) {
                    kfvKey(cur_db_item) = cur_obj_key;
                    for (json::iterator cur_obj_it = cur_obj.begin(); cur_obj_it != cur_obj.end(); cur_obj_it++)
                    {
                        string field_str = cur_obj_it.key();
                        string value_str;
                        if ((*cur_obj_it).is_number())
                            value_str = to_string((*cur_obj_it).get<int>());
                        else if ((*cur_obj_it).is_string())
                            value_str = (*cur_obj_it).get<string>();
                        kfvFieldsValues(cur_db_item).push_back(FieldValueTuple(field_str, value_str));
                    }
                }
                else
                {
                    if (op_name != child_it.key())
                    {
                        SWSS_LOG_ERROR("Invalid entry. %s", arr_item.dump().c_str());
                        return false;
                    }
                    kfvOp(cur_db_item) = cur_obj.get<string>();
                 }
            }
        }
        else
        {
            SWSS_LOG_ERROR("Child elements must be objects. element:%s", arr_item.dump().c_str());
            return false;
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    Logger::setMinPrio(Logger::SWSS_DEBUG);

    if (argc != 2)
    {
        usage(argv);
        exit(EXIT_FAILURE);
    }

    vector<KeyOpFieldsValuesTuple> db_items;
    fstream fs;
    try
    {
        fs.open(argv[1], fstream::in);
        if (!load_json_db_data(fs, db_items))
        {
            SWSS_LOG_ERROR("Failed loading data from json file.");
            fs.close();
            return EXIT_FAILURE;
        }
        fs.close();
    }
    catch(...)
    {
        cout << "Failed loading json file: " << argv[1] << " Please refer to logs\n";
        return EXIT_FAILURE;
    }

    try
    {
        if (!write_db_data(db_items))
        {
            SWSS_LOG_ERROR("Failed writing data to db\n");
            return EXIT_FAILURE;
        }
    }
    catch(...)
    {
        cout << "Failed applying settings from json file: " << argv[1] << " Please refer to logs\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}