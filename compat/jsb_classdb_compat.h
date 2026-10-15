#ifndef GODOTJS_CLASSDB_COMPAT_H
#define GODOTJS_CLASSDB_COMPAT_H

#include "jsb_engine_version_comparison.h"
#include "core/object/class_db.h"
#include "core/templates/list.h"
#if GODOT_4_7_OR_NEWER
#   include "core/object/gdtype.h"
#endif

namespace jsb::compat
{
    // enum constant names in declaration order, the same on every engine version
    struct EnumInfoCompat
    {
        List<StringName> constants;
        bool is_bitfield = false;
    };

    // Godot 4.7 moved the class name, parent name, signals, enums and constants from
    // ClassDB::ClassInfo into GDType. These helpers give the pre-4.7 view on every version:
    // only the members declared by the class itself, not inherited ones.
    struct ClassInfoCompat
    {
        static const StringName& get_name(const ClassDB::ClassInfo* p_class_info)
        {
#if GODOT_4_7_OR_NEWER
            return p_class_info->gdtype->get_name();
#else
            return p_class_info->name;
#endif
        }

        static const StringName& get_inherits(const ClassDB::ClassInfo* p_class_info)
        {
#if GODOT_4_7_OR_NEWER
            return p_class_info->gdtype->get_super_type_name();
#else
            return p_class_info->inherits;
#endif
        }

        // also contains the values of enum constants
        static const AHashMap<StringName, int64_t>& get_constant_map(const ClassDB::ClassInfo* p_class_info)
        {
#if GODOT_4_7_OR_NEWER
            return p_class_info->gdtype->get_integer_constant_map(true);
#else
            return p_class_info->constant_map;
#endif
        }

        static uint32_t get_enum_count(const ClassDB::ClassInfo* p_class_info)
        {
#if GODOT_4_7_OR_NEWER
            return p_class_info->gdtype->get_enum_map(true).size();
#else
            return p_class_info->enum_map.size();
#endif
        }

        static uint32_t get_signal_count(const ClassDB::ClassInfo* p_class_info)
        {
#if GODOT_4_7_OR_NEWER
            return p_class_info->gdtype->get_signal_map(true).size();
#else
            return p_class_info->signal_map.size();
#endif
        }

        // p_func(const StringName& signal_name, const MethodInfo& signal_info)
        template<typename TFunc>
        static void for_each_signal(const ClassDB::ClassInfo* p_class_info, TFunc&& p_func)
        {
#if GODOT_4_7_OR_NEWER
            for (const KeyValue<StringName, const MethodInfo*>& pair : p_class_info->gdtype->get_signal_map(true))
            {
                p_func(pair.key, *pair.value);
            }
#else
            for (const KeyValue<StringName, MethodInfo>& pair : p_class_info->signal_map)
            {
                p_func(pair.key, pair.value);
            }
#endif
        }

        // p_func(const StringName& enum_name, const EnumInfoCompat& enum_info)
        template<typename TFunc>
        static void for_each_enum(const ClassDB::ClassInfo* p_class_info, TFunc&& p_func)
        {
#if GODOT_4_7_OR_NEWER
            for (const KeyValue<StringName, const GDType::EnumInfo*>& pair : p_class_info->gdtype->get_enum_map(true))
            {
                EnumInfoCompat enum_info;
                enum_info.is_bitfield = pair.value->is_bitfield;
                for (const KeyValue<StringName, int64_t>& value : pair.value->values)
                {
                    enum_info.constants.push_back(value.key);
                }
                p_func(pair.key, enum_info);
            }
#else
            for (const KeyValue<StringName, ClassDB::ClassInfo::EnumInfo>& pair : p_class_info->enum_map)
            {
                EnumInfoCompat enum_info;
                enum_info.is_bitfield = pair.value.is_bitfield;
                for (const StringName& constant : pair.value.constants)
                {
                    enum_info.constants.push_back(constant);
                }
                p_func(pair.key, enum_info);
            }
#endif
        }
    };
}

#endif
