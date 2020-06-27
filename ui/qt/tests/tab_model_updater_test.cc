#include <cassert>
#include <iostream>
#include <vector>

#include <QAbstractTableModel>
#include <QTableView>
#include <QtTest>

#include "../widgets/tab_model_updater.h"

namespace {

struct Data {
    QString a, b;
};

// compare 'a' only to be able to trigger updates on data
int compare(const Data &d1, const Data &d2) {
    int result = d1.a.compare(d2.a);
    return result;
}

bool operator==(const Data &d1, const Data &d2) {
    return d1.a.compare(d2.a) == 0 && d1.b.compare(d2.b) == 0;
}

bool operator!=(const Data &a, const Data &b) {
    return !(a == b);
}

class TabView : public QObject {
    Q_OBJECT

    public:
        explicit TabView(const QAbstractTableModel &model) : model_(model) {
            connect(&model, &QAbstractTableModel::dataChanged, this, &TabView::onDataChanged);
            connect(&model, &QAbstractTableModel::rowsInserted, this, &TabView::onRowsInserted);
            connect(&model, &QAbstractTableModel::rowsRemoved, this, &TabView::onRowsRemoved);
        }

        const std::vector<Data> &data() const {
            return data_;
        }

    public slots:
        void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &) {
            for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
                for (int col = topLeft.column(); col <= bottomRight.column(); ++col) {
                    QVERIFY(col == 0 || col == 1);
                    QString &dest = col == 0 ? data_[static_cast<size_t>(row)].a : data_[static_cast<size_t>(row)].b;
                    dest = model_.data(model_.index(row, col)).toString();
                }
            }
        }

        void onRowsInserted(const QModelIndex &/*parent*/, int first, int last) {
            auto insert_iter = data_.begin() + first;

            for (;first <= last; ++first, ++insert_iter) {
                insert_iter = data_.insert(insert_iter, {
                    model_.data(model_.index(first, 0)).toString(),
                    model_.data(model_.index(first, 1)).toString()
                });
            }
        }

        void onRowsRemoved(const QModelIndex &/*parent*/, int first, int last) {
            data_.erase(data_.begin() + first, data_.begin() + last + 1);
        }

    private:
        QAbstractTableModel &model_;
        std::vector<Data> data_;
};

class TabModel : public QAbstractTableModel {
    Q_OBJECT

    public:
        virtual ~TabModel() = default;

        template<typename TAB_MODEL, typename TARGET_CONTAINER, typename SOURCE_CONTAINER, typename DATA_ITEM_COMP>
        friend void woinc::ui::qt::update_tab_model(TAB_MODEL &model,
                                                    TARGET_CONTAINER &current_data,
                                                    SOURCE_CONTAINER new_data,
                                                    DATA_ITEM_COMP comp);

        int rowCount(const QModelIndex &parent = QModelIndex()) const final override {
            return parent.isValid() ? 0 : static_cast<int>(data_.size());
        }

        int columnCount(const QModelIndex &parent = QModelIndex()) const final override {
            return parent.isValid() ? 0 : 2;
        }

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const final override {
            if (!index.isValid() || role != Qt::DisplayRole)
                return QVariant();

            assert(static_cast<size_t>(index.row()) < data_.size());
            assert(index.column() < 2);

            size_t row = static_cast<size_t>(index.row());
            return QVariant(index.column() == 0 ? data_[row].a : data_[row].b);
        }

        void update(std::vector<Data> new_data) {
            woinc::ui::qt::update_tab_model(*this, data_, std::move(new_data), &compare);
        }

        const std::vector<Data> &data() const {
            return data_;
        }

    private:
        std::vector<Data> data_;
};

void assert_row_count(TabView &view, int size) {
    QCOMPARE(static_cast<int>(view.data().size()), size);
}

void assert_content(TabView &view, int idx, const Data &data) {
    QCOMPARE(view.data()[static_cast<size_t>(idx)].a, data.a);
    QCOMPARE(view.data()[static_cast<size_t>(idx)].b, data.b);
}

void assert_equals(const std::vector<Data> &a, const std::vector<Data> &b) {
    QVERIFY(a.size() == b.size());
    QVERIFY(a == b);
}

}

class TestTabModelUpdater : public QObject {
    Q_OBJECT

    private slots:
        void insert_empty_single();
        void insert_empty_multiple();

        void insert_begin_single();
        void insert_begin_multiple();

        void insert_middle_single();
        void insert_middle_multiple();

        void insert_end_single();
        void insert_end_multiple();

        void update_single();
        void update_multiple();

        void remove_begin_single();
        void remove_begin_multiple();

        void remove_middle_single();
        void remove_middle_multiple();

        void remove_end_single();
        void remove_end_multiple();

        void remove_all();

        void mixed();
};

void TestTabModelUpdater::insert_empty_single() {
    TabModel model;
    TabView view(model);

    const Data data = { "A", "B" };

    model.update({data});

    assert_row_count(view, 1);
    assert_content(view, 0, data);
    assert_equals(model.data(), view.data());
}

void TestTabModelUpdater::insert_empty_multiple() {
    TabModel model;
    TabView view(model);

    const Data data1 = { "A", "B" };
    const Data data2 = { "C", "D" };
    const Data data3 = { "E", "F" };

    model.update({data1, data2, data3});

    assert_row_count(view, 3);
    assert_content(view, 0, data1);
    assert_content(view, 1, data2);
    assert_content(view, 2, data3);
    assert_equals(model.data(), view.data());
}

void TestTabModelUpdater::insert_begin_single() {
    TabModel model;
    TabView view(model);

    const Data data1 = { "A", "B" };
    const Data data2 = { "C", "D" };
    const Data data3 = { "E", "F" };

    model.update({data2, data3});
    model.update({data1, data2, data3});

    assert_row_count(view, 3);
    assert_content(view, 0, data1);
    assert_content(view, 1, data2);
    assert_content(view, 2, data3);
    assert_equals(model.data(), view.data());
}

void TestTabModelUpdater::insert_begin_multiple() {
    TabModel model;
    TabView view(model);

    const Data data1 = { "A", "B" };
    const Data data2 = { "C", "D" };
    const Data data3 = { "E", "F" };

    model.update({data3});
    model.update({data1, data2, data3});

    assert_row_count(view, 3);
    assert_content(view, 0, data1);
    assert_content(view, 1, data2);
    assert_content(view, 2, data3);
    assert_equals(model.data(), view.data());
}

void TestTabModelUpdater::insert_middle_single() {
    TabModel model;
    TabView view(model);

    const Data data1 = { "A", "B" };
    const Data data2 = { "C", "D" };
    const Data data3 = { "E", "F" };

    model.update({data1, data3});
    model.update({data1, data2, data3});

    assert_row_count(view, 3);
    assert_content(view, 0, data1);
    assert_content(view, 1, data2);
    assert_content(view, 2, data3);
    assert_equals(model.data(), view.data());
}

void TestTabModelUpdater::insert_middle_multiple() {
    TabModel model;
    TabView view(model);

    const Data data1 = { "A", "B" };
    const Data data2 = { "C", "D" };
    const Data data3 = { "E", "F" };
    const Data data4 = { "G", "H" };

    model.update({data1, data4});
    model.update({data1, data2, data3, data4});

    assert_row_count(view, 4);
    assert_content(view, 0, data1);
    assert_content(view, 1, data2);
    assert_content(view, 2, data3);
    assert_content(view, 3, data4);
    assert_equals(model.data(), view.data());
}

void TestTabModelUpdater::insert_end_single() {
    TabModel model;
    TabView view(model);

    const Data data1 = { "A", "B" };
    const Data data2 = { "C", "D" };
    const Data data3 = { "E", "F" };

    model.update({data1, data2});
    model.update({data1, data2, data3});

    assert_row_count(view, 3);
    assert_content(view, 0, data1);
    assert_content(view, 1, data2);
    assert_content(view, 2, data3);
    assert_equals(model.data(), view.data());
}

void TestTabModelUpdater::insert_end_multiple() {
    TabModel model;
    TabView view(model);

    const Data data1 = { "A", "B" };
    const Data data2 = { "C", "D" };
    const Data data3 = { "E", "F" };

    model.update({data1});
    model.update({data1, data2, data3});

    assert_row_count(view, 3);
    assert_content(view, 0, data1);
    assert_content(view, 1, data2);
    assert_content(view, 2, data3);
    assert_equals(model.data(), view.data());
}

void TestTabModelUpdater::update_single() {
    TabModel model;
    TabView view(model);

    const Data data_old = { "A", "B" };
    const Data data_new = { "new A", "new B" };

    model.update({data_old});
    model.update({data_new});

    assert_row_count(view, 1);
    assert_content(view, 0, data_new);
    assert_equals(model.data(), view.data());
}

void TestTabModelUpdater::update_multiple() {
    TabModel model;
    TabView view(model);

    const Data data_old1 = { "A", "B" };
    const Data data_old2 = { "C", "D" };
    const Data data_old3 = { "E", "F" };
    const Data data_old4 = { "G", "H" };
    const Data data_old5 = { "I", "J" };
    const Data data_old6 = { "K", "L" };

    const Data data_new1 = { "A", "new B" };
    const Data data_new2 = { "C", "D" };
    const Data data_new3 = { "E", "new F" };
    const Data data_new4 = { "G", "new H" };
    const Data data_new5 = { "I", "J" };
    const Data data_new6 = { "K", "new L" };

    model.update({data_old1, data_old2, data_old3, data_old4, data_old5, data_old6});
    model.update({data_new1, data_new2, data_new3, data_new4, data_new5, data_new6});

    assert_row_count(view, 6);
    assert_content(view, 0, data_new1);
    assert_content(view, 1, data_new2);
    assert_content(view, 2, data_new3);
    assert_content(view, 3, data_new4);
    assert_content(view, 4, data_new5);
    assert_content(view, 5, data_new6);
    assert_equals(model.data(), view.data());
}

void TestTabModelUpdater::remove_begin_single() {
    TabModel model;
    TabView view(model);

    const Data data1 = { "A", "B" };
    const Data data2 = { "C", "D" };
    const Data data3 = { "E", "F" };

    model.update({data1, data2, data3});
    model.update({data2, data3});

    assert_row_count(view, 2);
    assert_content(view, 0, data2);
    assert_content(view, 1, data3);
    assert_equals(model.data(), view.data());
}

void TestTabModelUpdater::remove_begin_multiple() {
    TabModel model;
    TabView view(model);

    const Data data1 = { "A", "B" };
    const Data data2 = { "C", "D" };
    const Data data3 = { "E", "F" };

    model.update({data1, data2, data3});
    model.update({data3});

    assert_row_count(view, 1);
    assert_content(view, 0, data3);
}

void TestTabModelUpdater::remove_middle_single() {
    TabModel model;
    TabView view(model);

    const Data data1 = { "A", "B" };
    const Data data2 = { "C", "D" };
    const Data data3 = { "E", "F" };

    model.update({data1, data2, data3});
    model.update({data1, data3});

    assert_row_count(view, 2);
    assert_content(view, 0, data1);
    assert_content(view, 1, data3);
}

void TestTabModelUpdater::remove_middle_multiple() {
    TabModel model;
    TabView view(model);

    const Data data1 = { "A", "B" };
    const Data data2 = { "C", "D" };
    const Data data3 = { "E", "F" };
    const Data data4 = { "G", "H" };

    model.update({data1, data2, data3, data4});
    model.update({data1, data4});

    assert_row_count(view, 2);
    assert_content(view, 0, data1);
    assert_content(view, 1, data4);
}

void TestTabModelUpdater::remove_end_single() {
    TabModel model;
    TabView view(model);

    const Data data1 = { "A", "B" };
    const Data data2 = { "C", "D" };
    const Data data3 = { "E", "F" };

    model.update({data1, data2, data3});
    model.update({data1, data2});

    assert_row_count(view, 2);
    assert_content(view, 0, data1);
    assert_content(view, 1, data2);
}

void TestTabModelUpdater::remove_end_multiple() {
    TabModel model;
    TabView view(model);

    const Data data1 = { "A", "B" };
    const Data data2 = { "C", "D" };
    const Data data3 = { "E", "F" };

    model.update({data1, data2, data3});
    model.update({data1});

    assert_row_count(view, 1);
    assert_content(view, 0, data1);
}

void TestTabModelUpdater::remove_all() {
    TabModel model;
    TabView view(model);

    const Data data1 = { "A", "B" };
    const Data data2 = { "C", "D" };
    const Data data3 = { "E", "F" };

    model.update({data1, data2, data3});
    model.update({});

    assert_row_count(view, 0);
}

void TestTabModelUpdater::mixed() {
    TabModel model;
    TabView view(model);

    const Data data_old1 = { "A", "B" };
    const Data data_old2 = { "C", "D" };
    const Data data_old3 = { "E", "F" };
    const Data data_old4 = { "G", "H" };
    const Data data_old5 = { "I", "J" };
    const Data data_old6 = { "K", "L" };

    const Data data_new1 = { "C", "D" };
    const Data data_new2 = { "F", "what ever" };
    const Data data_new3 = { "G", "new H" };
    const Data data_new4 = { "I", "J" };

    model.update({data_old1, data_old2, data_old3, data_old4, data_old5, data_old6});
    model.update({data_new1, data_new2, data_new3, data_new4});

    assert_row_count(view, 4);
    assert_content(view, 0, data_new1);
    assert_content(view, 1, data_new2);
    assert_content(view, 2, data_new3);
    assert_content(view, 3, data_new4);
    assert_equals(model.data(), view.data());
}

QTEST_MAIN(TestTabModelUpdater)
#include "tab_model_updater_test.moc"
