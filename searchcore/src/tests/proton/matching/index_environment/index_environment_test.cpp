// Copyright 2017 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/vespalib/testkit/test_kit.h>

#include <vespa/eval/eval/value_cache/constant_value.h>
#include <vespa/searchcore/proton/matching/indexenvironment.h>

using namespace proton::matching;
using search::fef::FieldInfo;
using search::fef::FieldType;
using search::fef::Properties;
using search::index::Schema;
using search::index::schema::CollectionType;
using search::index::schema::DataType;
using vespalib::eval::ConstantValue;
using SIAF = Schema::ImportedAttributeField;

struct MyConstantValueRepo : public IConstantValueRepo {
    virtual ConstantValue::UP getConstant(const vespalib::string &) const override {
        return ConstantValue::UP();
    }
};

Schema::UP
buildSchema()
{
    Schema::UP result = std::make_unique<Schema>();
    result->addImportedAttributeField(SIAF("imported_a", DataType::INT32, CollectionType::SINGLE));
    result->addImportedAttributeField(SIAF("imported_b", DataType::STRING, CollectionType::ARRAY));
    return result;
}

Schema::UP
buildEmptySchema()
{
    return std::make_unique<Schema>();
}

struct Fixture {
    MyConstantValueRepo repo;
    Schema::UP schema;
    IndexEnvironment env;
    Fixture(Schema::UP schema_)
        : repo(),
          schema(std::move(schema_)),
          env(*schema, Properties(), repo)
    {
    }
    const FieldInfo *assertField(size_t idx,
                                 const vespalib::string &name,
                                 DataType dataType,
                                 CollectionType collectionType) {
        const FieldInfo *field = env.getField(idx);
        ASSERT_TRUE(field != nullptr);
        EXPECT_EQUAL(field, env.getFieldByName(name));
        EXPECT_EQUAL(name, field->name());
        EXPECT_EQUAL(dataType, field->get_data_type());
        EXPECT_TRUE(collectionType == field->collection());
        EXPECT_EQUAL(idx, field->id());
        return field;
    }
    void assertHiddenAttributeField(size_t idx,
                                    const vespalib::string &name,
                                    DataType dataType,
                                    CollectionType collectionType) {
        const FieldInfo *field = assertField(idx, name, dataType, collectionType);
        EXPECT_FALSE(field->hasAttribute());
        EXPECT_TRUE(field->type() == FieldType::HIDDEN_ATTRIBUTE);
        EXPECT_TRUE(field->isFilter());
    }
    void assertAttributeField(size_t idx,
                              const vespalib::string &name,
                              DataType dataType,
                              CollectionType collectionType) {
        const FieldInfo *field = assertField(idx, name, dataType, collectionType);
        EXPECT_TRUE(field->hasAttribute());
        EXPECT_TRUE(field->type() == FieldType::ATTRIBUTE);
        EXPECT_FALSE(field->isFilter());
    }
};

TEST_F("require that document meta store is always extracted in index environment", Fixture(buildEmptySchema()))
{
    ASSERT_EQUAL(1u, f.env.getNumFields());
    TEST_DO(f.assertHiddenAttributeField(0, "[documentmetastore]", DataType::RAW, CollectionType::SINGLE));
}

TEST_F("require that imported attribute fields are extracted in index environment", Fixture(buildSchema()))
{
    ASSERT_EQUAL(3u, f.env.getNumFields());
    TEST_DO(f.assertAttributeField(0, "imported_a", DataType::INT32, CollectionType::SINGLE));
    TEST_DO(f.assertAttributeField(1, "imported_b", DataType::STRING, CollectionType::ARRAY));
    EXPECT_EQUAL("[documentmetastore]", f.env.getField(2)->name());
}

TEST_MAIN() { TEST_RUN_ALL(); }
