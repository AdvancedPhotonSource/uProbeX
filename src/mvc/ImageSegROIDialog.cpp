/*-----------------------------------------------------------------------------
 * Copyright (c) 2019, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include <mvc/ImageSegROIDialog.h>


#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QGridLayout>
#ifdef _BUILD_WITH_OPENCV
#include <opencv2/core/eigen.hpp>
#endif
//---------------------------------------------------------------------------
static const QString STR_KMEANS = QString("KMeans");
static const QString STR_DBSCAN = QString("DBSCAN");
static const QString STR_MANUAL = QString("Manual");

const int TAB_KMEANS = 0;
const int TAB_MANUAL = 1;

const int ACTION_ADD = 0;
const int ACTION_ERASE = 1;
const int ACTION_ADD_POLY = 2;
const int ACTION_ERASE_POLY = 3;

//---------------------------------------------------------------------------

ImageSegRoiDialog::ImageSegRoiDialog() : QDialog()
{

	_color_map.insert({ 0, QColor(Qt::red) });
	_color_map.insert({ 1, QColor(Qt::green) });
	_color_map.insert({ 2, QColor(Qt::blue) });
	_color_map.insert({ 3, QColor(Qt::yellow) });
	_color_map.insert({ 4, QColor(Qt::magenta) });
	_color_map.insert({ 5, QColor(Qt::white) });
	_color_map.insert({ 6, QColor(Qt::gray) });
	_color_map.insert({ 7, QColor(Qt::cyan) });
	_color_map.insert({ 8, QColor(Qt::darkGray) });
	_color_map.insert({ 9, QColor(Qt::darkGreen) });
	_color_map.insert({ 10, QColor(Qt::darkMagenta) });
	_color_map.insert({ 11, QColor(Qt::darkBlue) });
	_color_map.insert({ 12, QColor(Qt::darkRed) });
	_color_map.insert({ 13, QColor(Qt::darkYellow) });
	_color_map.insert({ 14, QColor(Qt::darkCyan) });
	_color_map.insert({ 15, QColor(Qt::black) });
	
	_layout_map[STR_KMEANS] = _createKMeansLayout();
	_layout_map[STR_DBSCAN] = _createDBScanLayout();
	_layout_map[STR_MANUAL] = _createManualLayout();
	_next_color = 0;
    createLayout();

}

//---------------------------------------------------------------------------

ImageSegRoiDialog::~ImageSegRoiDialog()
{

}

//---------------------------------------------------------------------------

void ImageSegRoiDialog::setImageData(std::unordered_map<std::string, data_struct::ArrayXXr<float>>& img_data)
{

	_img_list_model->clear();
	_img_data.clear();

	std::vector<std::string> element_lines;
	std::vector<std::string> scalers_to_add_first;
	std::vector<std::string> final_counts_to_add_before_scalers;
	gen_insert_order_lists(element_lines, scalers_to_add_first, final_counts_to_add_before_scalers);

	// insert in z order
	for (std::string el_name : element_lines)
	{
		if (img_data.count(el_name) > 0)
		{
			_img_data[QString(el_name.c_str())] = img_data.at(el_name);
			QStandardItem* item0 = new QStandardItem(false);
			item0->setCheckable(true);
			item0->setCheckState(Qt::Unchecked);
			item0->setText(QString(el_name.c_str()));
			_img_list_model->appendRow(item0);

			if (_image_size.isEmpty())
			{
				_image_size = QSize(img_data.at(el_name).cols(), img_data.at(el_name).rows());
			}
			img_data.erase(el_name);
		}

	}
	/*
	//add leftovers ( pile ups )
	for (auto& itr : element_counts_not_added)
	{
		// if it is not in the final add then add it
		if (std::find(final_counts_to_add_before_scalers.begin(), final_counts_to_add_before_scalers.end(), itr.first) == final_counts_to_add_before_scalers.end())
		{
			QString val = QString(itr.first.c_str());
			m_imageViewWidget->addLabel(val);
		}
	}
	*/
	// add end of element list that are not elements
	for (auto& itr : final_counts_to_add_before_scalers)
	{
		if (img_data.count(itr) > 0)
		{
			_img_data[QString(itr.c_str())] = img_data.at(itr);
			QStandardItem* item0 = new QStandardItem(false);
			item0->setCheckable(true);
			item0->setCheckState(Qt::Unchecked);
			item0->setText(QString(itr.c_str()));
			_img_list_model->appendRow(item0);

			img_data.erase(itr);
		}
	}

	// add scalers in certain order
	for (auto& itr : scalers_to_add_first)
	{
		if (img_data.count(itr) > 0)
		{
			_img_data[QString(itr.c_str())] = img_data.at(itr);
			QStandardItem* item0 = new QStandardItem(false);
			item0->setCheckable(true);
			item0->setCheckState(Qt::Unchecked);
			item0->setText(QString(itr.c_str()));
			_img_list_model->appendRow(item0);

			img_data.erase(itr);
		}
	}

	// add rest of scalers
	for (auto& itr : img_data)
	{
		_img_data[QString(itr.first.c_str())] = img_data.at(itr.first);
		QStandardItem* item0 = new QStandardItem(false);
		item0->setCheckable(true);
		item0->setCheckState(Qt::Unchecked);
		item0->setText(QString(itr.first.c_str()));
		_img_list_model->appendRow(item0);
	}


	/* // any order
	for (auto& itr : img_data)
	{
		//_img_data[QString(itr.first.c_str())] = itr.second;
		QStandardItem* item0 = new QStandardItem(false);
		item0->setCheckable(true);
		item0->setCheckState(Qt::Unchecked);
		item0->setText(QString(itr.first.c_str()));
		_img_list_model->appendRow(item0);
	}
	*/
}

//---------------------------------------------------------------------------

QWidget* ImageSegRoiDialog::_createKMeansLayout()
{
	QVBoxLayout* layout = new QVBoxLayout();

	QLabel* label = new QLabel("Number of Features:");
	_km_nfeatures = new QLineEdit();
	_km_nfeatures->setValidator(new QIntValidator(0, 10, this));
	_km_nfeatures->setText("2");

	QHBoxLayout* hlayout = new QHBoxLayout();
	hlayout->addWidget(label);
	hlayout->addWidget(_km_nfeatures);
	layout->addItem(hlayout);

	label = new QLabel("Term Criteria:");
	_km_TermCriteria = new QComboBox();
	_km_TermCriteria->addItem("COUNT");
	_km_TermCriteria->addItem("EPS"); 
	_km_TermCriteria->addItem("COUNT + EPS");
	_km_TermCriteria->setCurrentIndex(2); // default to EPS

	hlayout = new QHBoxLayout();
	hlayout->addWidget(label);
	hlayout->addWidget(_km_TermCriteria);
	layout->addItem(hlayout);

	label = new QLabel("Centers:");
	_km_Centers = new QComboBox();
	_km_Centers->addItem("Random");
	_km_Centers->addItem("PP");
	_km_Centers->setCurrentIndex(0); // default to Random

	hlayout = new QHBoxLayout();
	hlayout->addWidget(label);
	hlayout->addWidget(_km_Centers);
	layout->addItem(hlayout);
	
	label = new QLabel("Attempts:");
	_km_attempts = new QLineEdit();
	_km_attempts->setValidator(new QIntValidator(0, 100, this));
	_km_attempts->setText("3");

	hlayout = new QHBoxLayout();
	hlayout->addWidget(label);
	hlayout->addWidget(_km_attempts);
	layout->addItem(hlayout);

	label = new QLabel("MAX_Iter");
	_km_le_MAX_ITER = new QLineEdit();
	_km_le_MAX_ITER->setValidator(new QIntValidator(0, 10000, this));
	_km_le_MAX_ITER->setText("10");

	hlayout = new QHBoxLayout();
	hlayout->addWidget(label);
	hlayout->addWidget(_km_le_MAX_ITER);
	layout->addItem(hlayout);

	label = new QLabel("Epsilon");
	_km_le_epsilon = new QLineEdit();
	_km_le_epsilon->setValidator(new QDoubleValidator());
	_km_le_epsilon->setText("1.0");

	hlayout = new QHBoxLayout();
	hlayout->addWidget(label);
	hlayout->addWidget(_km_le_epsilon);
	layout->addItem(hlayout);
	
	_runBtn = new QPushButton("Run");
	connect(_runBtn, &QPushButton::pressed, this, &ImageSegRoiDialog::onRun);
	layout->addWidget(_runBtn);

	QWidget* widget = new QWidget();
	widget->setLayout(layout);
	return widget;
}

//---------------------------------------------------------------------------

QWidget* ImageSegRoiDialog::_createDBScanLayout()
{
	QVBoxLayout* layout = new QVBoxLayout();
/*
	QLabel* label = new QLabel("Number of Features:");
	_km_nfeatures = new QLineEdit();
	_km_nfeatures->setValidator(new QIntValidator(0, 10, this));
	_km_nfeatures->setText("2");

	QHBoxLayout* hlayout = new QHBoxLayout();
	hlayout->addWidget(label);
	hlayout->addWidget(_km_nfeatures);
	layout->addItem(hlayout);

	label = new QLabel("Term Criteria:");
	_km_TermCriteria = new QComboBox();
	_km_TermCriteria->addItem("COUNT");
	_km_TermCriteria->addItem("EPS"); 
	_km_TermCriteria->addItem("COUNT + EPS");
	_km_TermCriteria->setCurrentIndex(2); // default to EPS

	hlayout = new QHBoxLayout();
	hlayout->addWidget(label);
	hlayout->addWidget(_km_TermCriteria);
	layout->addItem(hlayout);

	label = new QLabel("Centers:");
	_km_Centers = new QComboBox();
	_km_Centers->addItem("Random");
	_km_Centers->addItem("PP");
	_km_Centers->setCurrentIndex(0); // default to Random

	hlayout = new QHBoxLayout();
	hlayout->addWidget(label);
	hlayout->addWidget(_km_Centers);
	layout->addItem(hlayout);
	
	label = new QLabel("Attempts:");
	_km_attempts = new QLineEdit();
	_km_attempts->setValidator(new QIntValidator(0, 100, this));
	_km_attempts->setText("3");

	hlayout = new QHBoxLayout();
	hlayout->addWidget(label);
	hlayout->addWidget(_km_attempts);
	layout->addItem(hlayout);

	label = new QLabel("MAX_Iter");
	_km_le_MAX_ITER = new QLineEdit();
	_km_le_MAX_ITER->setValidator(new QIntValidator(0, 10000, this));
	_km_le_MAX_ITER->setText("10");

	hlayout = new QHBoxLayout();
	hlayout->addWidget(label);
	hlayout->addWidget(_km_le_MAX_ITER);
	layout->addItem(hlayout);

	label = new QLabel("Epsilon");
	_km_le_epsilon = new QLineEdit();
	_km_le_epsilon->setValidator(new QDoubleValidator());
	_km_le_epsilon->setText("1.0");

	hlayout = new QHBoxLayout();
	hlayout->addWidget(label);
	hlayout->addWidget(_km_le_epsilon);
	layout->addItem(hlayout);
	
	_runBtn = new QPushButton("Run");
	connect(_runBtn, pressed, this, onRun);
	layout->addWidget(_runBtn);
*/
	QWidget* widget = new QWidget();
	widget->setLayout(layout);
	return widget;
}

//---------------------------------------------------------------------------

QWidget* ImageSegRoiDialog::_createManualLayout()
{
	QHBoxLayout* hlayout;
	QVBoxLayout* layout = new QVBoxLayout();

	//_cb_selected_roi = new QComboBox();
	_manual_btn_add_roi = new QPushButton("New ROI");
	connect(_manual_btn_add_roi, &QPushButton::pressed, this, &ImageSegRoiDialog::onNewROI);

	_manual_invert_roi = new QPushButton("Invert Selection");
	connect(_manual_invert_roi, &QPushButton::pressed, this, &ImageSegRoiDialog::onInvertROI);


	hlayout = new QHBoxLayout();
	//hlayout->addWidget(new QLabel("Selected ROI"));
	//hlayout->addWidget(_cb_selected_roi);
	hlayout->addWidget(_manual_btn_add_roi);
	layout->addItem(hlayout);


	_manual_cb_action = new QComboBox();
	_manual_cb_action->addItem("Brush Add");
	_manual_cb_action->addItem("Brush Erase");
	_manual_cb_action->addItem("Polygon Add");
	_manual_cb_action->addItem("Polygon Erase");
	connect(_manual_cb_action, &QComboBox::currentTextChanged, this, &ImageSegRoiDialog::manualActionChanged);

	hlayout = new QHBoxLayout();
	hlayout->addWidget(new QLabel("Action:"));
	hlayout->addWidget(_manual_cb_action);
	layout->addItem(hlayout);

	_manual_sp_brush_size = new QSpinBox();
	_manual_sp_brush_size->setRange(1, 100);
	_manual_sp_brush_size->setSingleStep(1.0);
	_manual_sp_brush_size->setValue(10);
	connect(_manual_sp_brush_size, qOverload<int>(&QSpinBox::valueChanged), this, &ImageSegRoiDialog::updateCustomCursor);

	hlayout = new QHBoxLayout();
	hlayout->addWidget(new QLabel("Brush Size"));
	hlayout->addWidget(_manual_sp_brush_size);
	layout->addItem(hlayout);

	layout->addWidget(_manual_invert_roi);

	QWidget* widget = new QWidget();
	widget->setLayout(layout);
	return widget;
}

//---------------------------------------------------------------------------

void ImageSegRoiDialog::createLayout()
{
	_techTabs = new QTabWidget();
	_techTabs->addTab(_layout_map[STR_KMEANS], STR_KMEANS);
	_techTabs->addTab(_layout_map[STR_DBSCAN], STR_DBSCAN);
	_techTabs->addTab(_layout_map[STR_MANUAL], STR_MANUAL);
	_techTabs->setEnabled(false);
	connect(_techTabs, &QTabWidget::currentChanged, this, &ImageSegRoiDialog::onTabChanged);

	_acceptBtn = new QPushButton("Accept");
	//_acceptBtn->setEnabled(false);
	_cancelBtn = new QPushButton("Cancel");
	connect(_acceptBtn, &QPushButton::pressed, this, &ImageSegRoiDialog::onAccept);
	connect(_cancelBtn, &QPushButton::pressed, this, &ImageSegRoiDialog::onClose);

	_img_list_model = new QStandardItemModel();
	_img_names_view = new QListView();
	_img_names_view->setModel(_img_list_model);
	_img_names_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
	_img_names_view->setSelectionMode(QAbstractItemView::ExtendedSelection);

	connect(_img_list_model, &QStandardItemModel::itemChanged, this, &ImageSegRoiDialog::onImgSelection);

	_int_img_widget = new ImageSegWidget();

	QHBoxLayout* mainLayout = new QHBoxLayout;
	QVBoxLayout *leftLayout = new QVBoxLayout;
	QHBoxLayout* optionLayout = new QHBoxLayout;
	QHBoxLayout* buttonLayout = new QHBoxLayout;

	_chk_normalize_sum = new QCheckBox("Normalize before summing");
	connect(_chk_normalize_sum, &QCheckBox::stateChanged, this, &ImageSegRoiDialog::onNormalizeChanged);

	optionLayout->addWidget(_chk_normalize_sum);

	buttonLayout->addWidget(_acceptBtn);
	buttonLayout->addWidget(_cancelBtn);
	
	leftLayout->addWidget(_img_names_view);
	leftLayout->addItem(optionLayout);
	leftLayout->addWidget(_techTabs);
	leftLayout->addItem(buttonLayout);

	mainLayout->addItem(leftLayout);
	mainLayout->addWidget(_int_img_widget);

	setLayout(mainLayout);

	setWindowTitle("ROI Image Segmentation");
}

//---------------------------------------------------------------------------

void ImageSegRoiDialog::onRun()
{
	if (_int_img_widget->getROIsCount() > 0)
	{
		QMessageBox msgBox;
		msgBox.setText("Clear Current ROI's?");
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);
		int ret = msgBox.exec();
		switch (ret) 
		{
			case QMessageBox::Yes:
				_int_img_widget->clearAllRoiMasks();
				break;
			default:
				break;
		}
	}

	ArrayXXr<float> int_img;
	_get_img(int_img, _chk_normalize_sum->isChecked());

	#ifdef _BUILD_WITH_OPENCV
	if(_techTabs->currentIndex() == 0) // KMEANS
	{
		cv::KmeansFlags flags;
		cv::TermCriteria crit;
		if (_km_TermCriteria->currentIndex() == 0)
		{
			crit.type = cv::TermCriteria::COUNT;
		}
		else if (_km_TermCriteria->currentIndex() == 1)
		{
			crit.type = cv::TermCriteria::EPS;
		}
		else if (_km_TermCriteria->currentIndex() == 2)
		{
			crit.type = cv::TermCriteria::COUNT | cv::TermCriteria::EPS;
		}
		crit.maxCount = _km_le_MAX_ITER->text().toInt();
		crit.epsilon = _km_le_epsilon->text().toDouble();
		int clusterCount = _km_nfeatures->text().toInt();
		int attempts = _km_attempts->text().toInt();

		if (_km_Centers->currentIndex() == 0)
		{
			flags = cv::KMEANS_RANDOM_CENTERS;
		}
		else if (_km_Centers->currentIndex() == 1)
		{
			flags = cv::KMEANS_PP_CENTERS;
		}

		cv::Mat labels, centers;
		
		cv::Mat data(int_img.rows() * int_img.cols(), 1, CV_32FC1, int_img.data());
		try
		{
			double compactness = cv::kmeans(data, clusterCount, labels, crit, attempts, flags, centers);
		}
		catch (std::exception& e)
		{
			QMessageBox::critical(nullptr, "ImageSegROIDialog Error", e.what());
			_acceptBtn->setEnabled(true);
			return;
		}
		cv::Mat new_labels = labels.reshape(1, int_img.rows());
		

		for (int i = 0; i < clusterCount; i++)
		{
			int color_idx = i;
			if (color_idx > _color_map.size())
			{
				color_idx = color_idx % _color_map.size();
			}
			QColor color_data = _color_map[color_idx];
			_next_color++;
			gstar::RoiMaskGraphicsItem* roi = new gstar::RoiMaskGraphicsItem(new_labels, i, color_data);
			_int_img_widget->addRoiMask(roi);
		}
	}
	else if(_techTabs->currentIndex() == 1) // DBSCAN
	{
		/*
		int slices=8;
		bool merge_small=true;
		cv::Ptr< cv::ximgproc::ScanSegment > scanSeg = cv::ximgproc::createScanSegment(int_img.cols(), int_img.rows(), 1, slices=8, merge_small);
		*/
	}
	#endif
	_acceptBtn->setEnabled(true);
}

//---------------------------------------------------------------------------

void ImageSegRoiDialog::onAccept()
{
	// emit list of roi's
	emit onNewROIs(_int_img_widget->getAllROIs());
	close();
}

//---------------------------------------------------------------------------

void ImageSegRoiDialog::onClose()
{
	clear_all_rois();
	clear_image();
	_techTabs->setEnabled(false);
	close();
}

//---------------------------------------------------------------------------

void ImageSegRoiDialog::onNewROI()
{
	QColor color_data = _color_map[_next_color];
	ArrayXXr<float> int_img;
	if (_get_img(int_img, _chk_normalize_sum->isChecked()))
	{
		gstar::RoiMaskGraphicsItem* roi = new gstar::RoiMaskGraphicsItem(int_img.rows(), int_img.cols(), color_data);
		_int_img_widget->addRoiMask(roi);
		_next_color++;
	}
}

//---------------------------------------------------------------------------

void ImageSegRoiDialog::onInvertROI()
{
	_int_img_widget->invertSelectedRoiMask();
}

//---------------------------------------------------------------------------

void ImageSegRoiDialog::updateCustomCursor(int val)
{
	
	QPixmap pmap(val, val);
	if (_manual_cb_action->currentIndex() == ACTION_ADD)
	{
		_int_img_widget->setRoiBrushSize(val);
		pmap.fill(QColor(Qt::green));
		_int_img_widget->setActionMode(gstar::DRAW_ACTION_MODES::ADD_DRAW);
	}
	else if (_manual_cb_action->currentIndex() == ACTION_ERASE)
	{
		_int_img_widget->setRoiBrushSize(val);
		pmap.fill(QColor(Qt::red));
		_int_img_widget->setActionMode(gstar::DRAW_ACTION_MODES::ERASE_DRAW);
	}
	else if (_manual_cb_action->currentIndex() == ACTION_ADD_POLY)
	{
		_int_img_widget->setRoiBrushSize(0);
		pmap.fill(QColor(Qt::green));
		_int_img_widget->setActionMode(gstar::DRAW_ACTION_MODES::ADD_POLY);
	}
	else if (_manual_cb_action->currentIndex() == ACTION_ERASE_POLY)
	{
		_int_img_widget->setRoiBrushSize(0);
		pmap.fill(QColor(Qt::red));
		_int_img_widget->setActionMode(gstar::DRAW_ACTION_MODES::ERASE_POLY);
	}
	
	_int_img_widget->imageViewWidget()->customCursor(QCursor(pmap));
}

//---------------------------------------------------------------------------

void ImageSegRoiDialog::onTabChanged(int tab)
{
	if (tab == TAB_MANUAL)
	{
		int bs = _manual_sp_brush_size->value();
		updateCustomCursor(bs);
	}
	else
	{
		_int_img_widget->imageViewWidget()->clickCursor();
		_int_img_widget->setActionMode(gstar::DRAW_ACTION_MODES::OFF);
	}
}

//---------------------------------------------------------------------------

void ImageSegRoiDialog::manualActionChanged(QString)
{
	int bs = _manual_sp_brush_size->value();
	updateCustomCursor(bs);
}

//---------------------------------------------------------------------------

void ImageSegRoiDialog::clear_all_rois()
{
	//_int_img_widget->clearAllRoiMasks();
	_int_img_widget->deleteAllItems();
	//_cb_selected_roi->clear();
}

//---------------------------------------------------------------------------

void ImageSegRoiDialog::setColorMap(QVector<QRgb>* selected_colormap)
{
	_selected_colormap = selected_colormap;
}

//---------------------------------------------------------------------------

bool ImageSegRoiDialog::_get_img(ArrayXXr<float> &int_img, bool normalize)
{
	bool first = true;

	if (normalize)
	{
		float highCoef = 0.;
		ArrayXXr<float>* tmp_img;
		// go through and get max and min value  
		for (int i = 0; i < _img_list_model->rowCount(); i++)
		{
			QStandardItem* item0 = _img_list_model->item(i);
			if (item0->checkState() == Qt::Checked && _img_data.count(item0->text()) > 0)
			{
				tmp_img = &_img_data.at(item0->text());
				highCoef = std::max(tmp_img->maxCoeff(), highCoef);
			}
		}

		// go through and normalize before summing
		for (int i = 0; i < _img_list_model->rowCount(); i++)
		{
			QStandardItem* item0 = _img_list_model->item(i);
			if (item0->checkState() == Qt::Checked && _img_data.count(item0->text()) > 0)
			{
				ArrayXXr<float> norm_img = _img_data.at(item0->text());
				float localLow = norm_img.minCoeff();
				float localDiff = norm_img.maxCoeff() - localLow;

				norm_img = norm_img.unaryExpr([localLow, localDiff, highCoef](float v) { return ((v - localLow)/ (localDiff) ) * highCoef; });

				if (first)
				{
					int_img = norm_img;
					first = false;
				}
				else
				{
					int_img += norm_img;
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < _img_list_model->rowCount(); i++)
		{
			QStandardItem* item0 = _img_list_model->item(i);
			if (item0->checkState() == Qt::Checked && _img_data.count(item0->text()) > 0)
			{
				if (first)
				{
					int_img = _img_data.at(item0->text());
					first = false;
				}
				else
				{
					int_img += _img_data.at(item0->text());
				}
			}
		}
	}
	return !first; 
}

//---------------------------------------------------------------------------
/*
std::vector<QImage> ImageSegRoiDialog::_generate_images(int num_images, cv::Mat& mat)
{
	std::vector<QImage> images;
	for (int i = 0; i < num_images; i++)
	{
		QImage image(mat.cols, mat.rows, QImage::Format_ARGB32);
		image.setColorTable(*_selected_colormap);
		images.emplace_back(image);
	}

	for (int row = 0; row < mat.rows; row++)
	{
		for (int col = 0; col < mat.cols; col++)
		{
			//first clamp the data to max min
			int idx = mat.at<int>(row, col);
			byte data = 255;
			images[idx].setPixel(col, row, (uint)data);
		}
	}
	return images;
}

//---------------------------------------------------------------------------

QImage ImageSegRoiDialog::_generate_sum_image(cv::Mat& mat, ArrayXXr<float>& bg_img, uchar alpha)
{

	QImage background = _generate_img(bg_img).convertToFormat(QImage::Format_ARGB32_Premultiplied);
	
	QImage overlay(mat.cols, mat.rows, QImage::Format_ARGB32);

	for (int row = 0; row < mat.rows; row++)
	{
		for (int col = 0; col < mat.cols; col++)
		{
			//first clamp the data to max min
			int color_idx = mat.at<int>(row, col);
			if (color_idx > _color_map.size())
			{
				color_idx = color_idx % _color_map.size();
			}
			QColor color_data = _color_map[color_idx];
			color_data.setAlpha(alpha);

			//QRgb val = background.pixel(col, row);
			//val = color_data.rgba();
			overlay.setPixel(col, row, color_data.rgba());
		}
	}

	QPainter p(&background);
	//p.setCompositionMode(QPainter::CompositionMode_SourceOver);
	p.setCompositionMode(QPainter::CompositionMode_Plus);
	//p.setCompositionMode(QPainter::CompositionMode_Exclusion);
	//p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
	p.drawImage(0, 0, overlay);
	p.end();
	
	return background;
	
}
*/
//---------------------------------------------------------------------------

void ImageSegRoiDialog::onNormalizeChanged(int a)
{
	onImgSelection(nullptr);
}

//---------------------------------------------------------------------------

void ImageSegRoiDialog::onImgSelection(QStandardItem* item) 
{
	ArrayXXr<float> int_img;
	bool is_checked = _get_img(int_img, _chk_normalize_sum->isChecked());

	_techTabs->setEnabled(is_checked);

	if (is_checked)
	{
		_int_img_widget->setImageFromArray(int_img, *_selected_colormap);
	}
	else if (false == _image_size.isEmpty())
	{
		clear_image();
	}
}

//---------------------------------------------------------------------------

void ImageSegRoiDialog::clear_image()
{
	QImage image(_image_size, QImage::Format_ARGB32_Premultiplied);
	image.fill(QColor(Qt::gray));
	_int_img_widget->setPixMap(QPixmap::fromImage(image.convertToFormat(QImage::Format_RGB32)));
}

//---------------------------------------------------------------------------