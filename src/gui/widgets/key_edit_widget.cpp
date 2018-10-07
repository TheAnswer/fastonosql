/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

    This file is part of FastoNoSQL.

    FastoNoSQL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoNoSQL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoNoSQL.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gui/widgets/key_edit_widget.h"

#include <QComboBox>
#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

#include <Qsci/qscilexerjson.h>

#include <common/convert2string.h>
#include <common/qt/convert2string.h>  // for ConvertToString

#include <fastonosql/core/value.h>

#include "gui/widgets/hash_type_widget.h"
#include "gui/widgets/list_type_widget.h"
#include "gui/widgets/stream_type_widget.h"

#include "gui/gui_factory.h"  // for GuiFactory
#include "gui/hash_table_model.h"

#include "gui/editor/fasto_editor.h"

#include "translations/global.h"  // for trAddItem, trRemoveItem, etc

namespace fastonosql {
namespace gui {

KeyEditWidget::KeyEditWidget(const std::vector<common::Value::Type>& availible_types, QWidget* parent)
    : base_class(parent) {
  QGridLayout* kvLayout = new QGridLayout;
  type_label_ = new QLabel;
  kvLayout->addWidget(type_label_, 0, 0);
  types_combo_box_ = new QComboBox;
  for (size_t i = 0; i < availible_types.size(); ++i) {
    common::Value::Type t = availible_types[i];
    QString type = core::GetTypeName(t);
    types_combo_box_->addItem(GuiFactory::GetInstance().icon(t), type, t);
  }

  typedef void (QComboBox::*ind)(int);
  VERIFY(
      connect(types_combo_box_, static_cast<ind>(&QComboBox::currentIndexChanged), this, &KeyEditWidget::changeType));
  kvLayout->addWidget(types_combo_box_, 0, 1);

  // key layout
  key_label_ = new QLabel;
  kvLayout->addWidget(key_label_, 1, 0);
  key_edit_ = new QLineEdit;
  key_edit_->setPlaceholderText("[key]");
  kvLayout->addWidget(key_edit_, 1, 1);

  // value layout
  value_label_ = new QLabel;
  kvLayout->addWidget(value_label_, 2, 0);
  value_edit_ = new QLineEdit;
  value_edit_->setPlaceholderText("[value]");
  kvLayout->addWidget(value_edit_, 2, 1);
  value_edit_->setVisible(true);
  VERIFY(connect(value_edit_, &QLineEdit::textChanged, this, &KeyEditWidget::keyChanged));

  json_value_edit_ = new FastoEditor;
  QsciLexerJSON* json_lexer = new QsciLexerJSON;
  json_value_edit_->setLexer(json_lexer);
  kvLayout->addWidget(json_value_edit_, 2, 1);
  json_value_edit_->setVisible(false);
  VERIFY(connect(json_value_edit_, &FastoEditor::textChanged, this, &KeyEditWidget::keyChanged));

  bool_value_edit_ = new QComboBox;
  bool_value_edit_->addItem("true");
  bool_value_edit_->addItem("false");
  kvLayout->addWidget(bool_value_edit_, 2, 1);
  bool_value_edit_->setVisible(false);
  VERIFY(connect(bool_value_edit_, &QComboBox::currentTextChanged, this, &KeyEditWidget::keyChanged));

  value_list_edit_ = new ListTypeWidget;
  // value_list_edit_->horizontalHeader()->hide();
  // value_list_edit_->verticalHeader()->hide();
  kvLayout->addWidget(value_list_edit_, 2, 1);
  value_list_edit_->setVisible(false);
  VERIFY(connect(value_list_edit_, &ListTypeWidget::dataChanged, this, &KeyEditWidget::keyChanged));

  value_table_edit_ = new HashTypeWidget;
  // value_table_edit_->horizontalHeader()->hide();
  // value_table_edit_->verticalHeader()->hide();
  kvLayout->addWidget(value_table_edit_, 2, 1);
  value_table_edit_->setVisible(false);
  VERIFY(connect(value_table_edit_, &HashTypeWidget::dataChanged, this, &KeyEditWidget::keyChanged));

  stream_table_edit_ = new StreamTypeWidget;
  kvLayout->addWidget(stream_table_edit_, 2, 1);
  stream_table_edit_->setVisible(false);
  VERIFY(connect(stream_table_edit_, &StreamTypeWidget::dataChanged, this, &KeyEditWidget::keyChanged));

  setLayout(kvLayout);
  retranslateUi();
}

void KeyEditWidget::initialize(const core::NDbKValue& key) {
  core::NValue val = key.GetValue();
  CHECK(val) << "Value of key must be inited!";
  common::Value::Type current_type = key.GetType();

  int current_index = -1;
  for (int i = 0; i < types_combo_box_->count(); ++i) {
    QVariant cur = types_combo_box_->itemData(i);
    common::Value::Type type = static_cast<common::Value::Type>(qvariant_cast<unsigned char>(cur));
    if (current_type == type) {
      current_index = static_cast<int>(i);
    }
  }

  CHECK_NE(current_index, -1) << "Type should be in availible_types array!";

  // sync keyname box
  QString qkey;
  core::NKey nkey = key.GetKey();
  core::key_t raw_key = nkey.GetKey();
  if (common::ConvertFromString(raw_key.GetHumanReadable(), &qkey)) {
    key_edit_->setText(qkey);
  }

  types_combo_box_->setCurrentIndex(current_index);
  syncControls(val);
}

KeyEditWidget::~KeyEditWidget() {}

void KeyEditWidget::setEnableKeyEdit(bool key_edit) {
  key_edit_->setEnabled(key_edit);
}

common::Value* KeyEditWidget::createItem() const {
  int index = types_combo_box_->currentIndex();
  QVariant var = types_combo_box_->itemData(index);
  common::Value::Type type = static_cast<common::Value::Type>(qvariant_cast<unsigned char>(var));
  if (type == common::Value::TYPE_ARRAY) {
    return value_list_edit_->arrayValue();
  } else if (type == common::Value::TYPE_SET) {
    return value_list_edit_->setValue();
  } else if (type == common::Value::TYPE_ZSET) {
    return value_table_edit_->zsetValue();
  } else if (type == common::Value::TYPE_HASH) {
    return value_table_edit_->hashValue();
  } else if (type == core::StreamValue::TYPE_STREAM) {
    return stream_table_edit_->streamValue();
  } else if (type == core::JsonValue::TYPE_JSON) {
    const std::string text_str = common::ConvertToString(json_value_edit_->text());
    if (!core::JsonValue::IsValidJson(text_str)) {
      return nullptr;
    }
    return new core::JsonValue(text_str);
  } else if (type == common::Value::TYPE_BOOLEAN) {
    int index = bool_value_edit_->currentIndex();
    return common::Value::CreateBooleanValue(index == 0);
  }

  const std::string text_str = common::ConvertToString(value_edit_->text());
  if (text_str.empty()) {
    return nullptr;
  }

  if (type == common::Value::TYPE_INTEGER) {
    int res;
    bool is_ok = common::ConvertFromString(text_str, &res);
    if (!is_ok) {
      DNOTREACHED() << "Conversion to int failed, text: " << text_str;
      return nullptr;
    }
    return common::Value::CreateIntegerValue(res);
  } else if (type == common::Value::TYPE_UINTEGER) {
    unsigned int res;
    bool is_ok = common::ConvertFromString(text_str, &res);
    if (!is_ok) {
      DNOTREACHED() << "Conversion to unsigned int failed, text: " << text_str;
      return nullptr;
    }
    return common::Value::CreateUIntegerValue(res);
  } else if (type == common::Value::TYPE_DOUBLE) {
    double res;
    bool is_ok = common::ConvertFromString(text_str, &res);
    if (!is_ok) {
      DNOTREACHED() << "Conversion to double failed, text: " << text_str;
      return nullptr;
    }
    return common::Value::CreateDoubleValue(res);
  }

  return common::Value::CreateStringValue(text_str);
}

void KeyEditWidget::changeEvent(QEvent* e) {
  if (e->type() == QEvent::LanguageChange) {
    retranslateUi();
  }
  base_class::changeEvent(e);
}

void KeyEditWidget::changeType(int index) {
  QVariant var = types_combo_box_->itemData(index);
  common::Value::Type type = static_cast<common::Value::Type>(qvariant_cast<unsigned char>(var));

  value_edit_->clear();
  json_value_edit_->clear();
  value_table_edit_->clear();
  stream_table_edit_->clear();
  value_list_edit_->clear();

  if (type == common::Value::TYPE_ARRAY || type == common::Value::TYPE_SET) {
    value_list_edit_->setVisible(true);
    value_edit_->setVisible(false);
    json_value_edit_->setVisible(false);
    bool_value_edit_->setVisible(false);
    value_table_edit_->setVisible(false);
    stream_table_edit_->setVisible(false);
  } else if (type == common::Value::TYPE_ZSET || type == common::Value::TYPE_HASH) {
    value_table_edit_->setVisible(true);
    stream_table_edit_->setVisible(false);
    value_edit_->setVisible(false);
    json_value_edit_->setVisible(false);
    bool_value_edit_->setVisible(false);
    value_list_edit_->setVisible(false);
  } else if (type == common::Value::TYPE_BOOLEAN) {
    value_table_edit_->setVisible(false);
    stream_table_edit_->setVisible(false);
    value_edit_->setVisible(false);
    json_value_edit_->setVisible(false);
    bool_value_edit_->setVisible(true);
    value_list_edit_->setVisible(false);
  } else if (type == core::StreamValue::TYPE_STREAM) {
    value_table_edit_->setVisible(false);
    stream_table_edit_->setVisible(true);
    value_edit_->setVisible(false);
    json_value_edit_->setVisible(false);
    bool_value_edit_->setVisible(false);
    value_list_edit_->setVisible(false);
  } else if (type == core::JsonValue::TYPE_JSON) {
    value_table_edit_->setVisible(false);
    stream_table_edit_->setVisible(false);
    value_edit_->setVisible(false);
    json_value_edit_->setVisible(true);
    bool_value_edit_->setVisible(false);
    value_list_edit_->setVisible(false);
  } else {
    value_edit_->setVisible(true);
    json_value_edit_->setVisible(false);
    bool_value_edit_->setVisible(false);
    value_list_edit_->setVisible(false);
    value_table_edit_->setVisible(false);
    stream_table_edit_->setVisible(false);
    if (type == common::Value::TYPE_INTEGER || type == common::Value::TYPE_UINTEGER) {
      value_edit_->setValidator(new QIntValidator(this));
    } else if (type == common::Value::TYPE_DOUBLE) {
      value_edit_->setValidator(new QDoubleValidator(this));
    } else {
      QRegExp rx(".*");
      value_edit_->setValidator(new QRegExpValidator(rx, this));
    }
  }

  emit typeChanged(type);
}

void KeyEditWidget::syncControls(const core::NValue& item) {
  if (!item) {
    return;
  }

  common::Value::Type t = item->GetType();
  if (t == common::Value::TYPE_ARRAY) {
    common::ArrayValue* arr = nullptr;
    if (item->GetAsList(&arr)) {
      for (auto it = arr->begin(); it != arr->end(); ++it) {
        std::string val = core::ConvertValue(*it, DEFAULT_DELIMITER);
        if (val.empty()) {
          continue;
        }

        QString qval;
        if (common::ConvertFromString(val, &qval)) {
          value_list_edit_->insertRow(qval);
        }
      }
    }
  } else if (t == common::Value::TYPE_SET) {
    common::SetValue* set = nullptr;
    if (item->GetAsSet(&set)) {
      for (auto it = set->begin(); it != set->end(); ++it) {
        std::string val = core::ConvertValue(*it, DEFAULT_DELIMITER);
        if (val.empty()) {
          continue;
        }

        QString qval;
        if (common::ConvertFromString(val, &qval)) {
          value_list_edit_->insertRow(qval);
        }
      }
    }
  } else if (t == common::Value::TYPE_ZSET) {
    common::ZSetValue* zset = nullptr;
    if (item->GetAsZSet(&zset)) {
      for (auto it = zset->begin(); it != zset->end(); ++it) {
        auto element = (*it);
        common::Value* key = element.first;
        std::string key_str = core::ConvertValue(key, DEFAULT_DELIMITER);
        if (key_str.empty()) {
          continue;
        }

        common::Value* value = element.second;
        std::string value_str = core::ConvertValue(value, DEFAULT_DELIMITER);
        if (value_str.empty()) {
          continue;
        }

        QString ftext;
        QString stext;
        if (common::ConvertFromString(key_str, &ftext) && common::ConvertFromString(value_str, &stext)) {
          value_table_edit_->insertRow(ftext, stext);
        }
      }
    }
  } else if (t == common::Value::TYPE_HASH) {
    common::HashValue* hash = nullptr;
    if (item->GetAsHash(&hash)) {
      for (auto it = hash->begin(); it != hash->end(); ++it) {
        auto element = (*it);
        common::Value* key = element.first;
        std::string key_str = core::ConvertValue(key, DEFAULT_DELIMITER);
        if (key_str.empty()) {
          continue;
        }

        common::Value* value = element.second;
        std::string value_str = core::ConvertValue(value, DEFAULT_DELIMITER);
        if (value_str.empty()) {
          continue;
        }

        QString ftext;
        QString stext;
        if (common::ConvertFromString(key_str, &ftext) && common::ConvertFromString(value_str, &stext)) {
          value_table_edit_->insertRow(ftext, stext);
        }
      }
    }
  } else if (t == core::StreamValue::TYPE_STREAM) {
    core::StreamValue* stream = static_cast<core::StreamValue*>(item.get());
    auto entr = stream->GetStreams();
    for (size_t i = 0; i != entr.size(); ++i) {
      auto ent = entr[i];
      stream_table_edit_->insertStream(ent);
    }
  } else if (t == core::JsonValue::TYPE_JSON) {
    std::string text;
    if (item->GetAsString(&text)) {
      QString qval;
      if (common::ConvertFromString(text, &qval)) {
        json_value_edit_->setText(qval);
      }
    }
  } else if (t == common::Value::TYPE_BOOLEAN) {
    bool val;
    if (item->GetAsBoolean(&val)) {
      bool_value_edit_->setCurrentIndex(val ? 0 : 1);
    }
  } else {
    std::string text;
    if (item->GetAsString(&text)) {
      QString qval;
      if (common::ConvertFromString(text, &qval)) {
        value_edit_->setText(qval);
      }
    }
  }
}

bool KeyEditWidget::getKey(core::NDbKValue* key) const {
  if (!key) {
    return false;
  }

  QString key_name = key_edit_->text();
  if (key_name.isEmpty()) {
    return false;
  }

  common::Value* obj = createItem();
  if (!obj) {
    return false;
  }

  core::key_t ks = common::ConvertToString(key_name);
  *key = core::NDbKValue(core::NKey(ks), core::NValue(obj));
  return true;
}

void KeyEditWidget::retranslateUi() {
  value_label_->setText(translations::trValue + ":");
  key_label_->setText(translations::trKey + ":");
  type_label_->setText(translations::trType + ":");
}

}  // namespace gui
}  // namespace fastonosql
