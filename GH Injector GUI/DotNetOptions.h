/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

#pragma once

#include "pch.h"

#include "framelesswindow.h"

class DotNetOptionsTree
{
	QString m_Option;
	std::vector<DotNetOptionsTree *> m_Nodes;

public:
	DotNetOptionsTree();
	DotNetOptionsTree(const QString & option);
	~DotNetOptionsTree();

	void ParseData(const QString & Data);

	const std::vector<DotNetOptionsTree *> GetOptions() const;
	const QString & GetData() const;

	DotNetOptionsTree * Search(const QString & option) const;
};

class DotNetOptionsWindow : public QDialog
{
	Q_OBJECT

	const DotNetOptionsTree * m_pOptionsRoot;

	FramelessWindow * m_FramelessParent = nullptr;

	QComboBox * cmb_Namespace = nullptr;
	QComboBox * cmb_Classname = nullptr;
	QComboBox * cmb_Methodname = nullptr;

	QLineEdit * txt_Namespace = nullptr;
	QLineEdit * txt_Classname = nullptr;
	QLineEdit * txt_Methodname = nullptr;

	QLineEdit * txt_Argument = nullptr;

	QCheckBox * cb_Entrypoint = nullptr;

	std::vector<QString> m_Results;
	bool m_UseNative = false;
	
public:
	DotNetOptionsWindow(const QString & title, const QStringList & options, const DotNetOptionsTree * root, bool use_native = false, QWidget * parent = nullptr);
	~DotNetOptionsWindow();

	void GetResults(std::vector<QString> & results, bool & use_native);
	void GetResult(QString & result, UINT index);

protected:
	bool eventFilter(QObject * obj, QEvent * event) override;

private slots:
	void on_close_button_clicked();
	void on_save_button_clicked();
	void on_native_changed();

	void on_namespace_change(int index);
	void on_classname_change(int index);

public slots:
	void show();
};