#ifndef RHE_WIDGET_H
    #define RHE_WIDGET_H

    #include <QDir>
    #include <QPainter>
    #include <QDirIterator>
    #include <QResizeEvent>
    #include <QXmlStreamReader>
    #include "general_widget.h"
    #include "send_recieve_module.h"

    QT_BEGIN_NAMESPACE
    namespace Ui {
        class RHE_Widget;
    }
    QT_END_NAMESPACE

    class RHE_Widget : public QWidget {
        Q_OBJECT
        public:
            RHE_Widget(QWidget *parent = nullptr, General_Widget *widg = nullptr, Send_Recieve_Module *snd_rcv_mod = nullptr);
            ~RHE_Widget() override;

            void pshBttn_set_path_to_proj_set_visible(bool flag);
            void pshBttn_chk_prj_stat_set_visible(bool flag);
            void pshBttn_ld_frmwr_set_visible(bool flag);
            void pshBttn_chk_prj_stat_set_enabled(bool flag);
            void pshBttn_ld_frmwr_set_enabled(bool flag);
            void pshBttn_snd_frmwr_set_enabled(bool flag);
            void set_fname_lname(QString str);

            bool sof_exist = false;
            bool svf_exist = false;
            bool pins_chk = false;

        private:
            void initialize_ui();
            void set_ui_text();
            void check_is_proj_folder(bool folder_exist);
            bool read_xml_file(bool type, QString *cur_fpga = nullptr, QList<QString> *pins_numb = nullptr, QList<QString> *pins_typ = nullptr, QList<QString> *pins_io_stndrt = nullptr);
            bool check_fpga_connections(QString path_to_fit_rprtr);

            Ui::RHE_Widget *ui;
            General_Widget *gen_widg = nullptr;
            Send_Recieve_Module *snd_rcv_module = nullptr; 
            QString *path_to_proj = nullptr;
            QString *prev_path_to_proj = nullptr;
            QFile *svf_file = nullptr;
            QList<QString> *pixmp_names = nullptr;
            QList<QString> *jtag_id_codes = nullptr;
            QString lname_fname;
            QPixmap pixmp_brd;

            bool qpf_exist = false;
            bool fit_exist = false;
            bool path_exist = false;
            bool ui_initialized = false;

            bool tmp_clr = false;

        public slots:
            void resizeEvent(QResizeEvent *) override;
            void slot_re_translate();

        private slots:
            void showEvent(QShowEvent *) override;
            void paintEvent(QPaintEvent *) override;
            void on_pushButton_clicked();
            void on_cmbBx_chs_brd_currentIndexChanged(int index);
            void on_pshBttn_set_path_to_proj_clicked();
            void on_pshBttn_snd_frmwr_clicked();
            void on_pshBttn_chk_prj_stat_clicked();
            void on_pshBttn_ld_frmwr_clicked();
            void on_pushButton_2_clicked();
            void on_pushButton_3_clicked();
    };

#endif // RHE_WIDGET_H
