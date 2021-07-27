/*-----------------------------------------------------------------------------
 * Copyright (c) 2021, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include "gstar/ContrastDialog.h"

using namespace gstar;
/*---------------------------------------------------------------------------*/

ContrastDialog::ContrastDialog(const data_struct::ArrayXXr* arr, QWidget* parent) : QDialog(parent)
{

	_historgram_widget = new QWidget(this);
	_historgram_widget->setMinimumSize(250, 50);
	_historgram = new HistogramPlot(_historgram_widget);

	_min_max_slider = new MinMaxSlider(false);
	connect(_min_max_slider, &MinMaxSlider::min_val_changed, this, &ContrastDialog::min_max_update);
	connect(_min_max_slider, &MinMaxSlider::max_val_changed, this, &ContrastDialog::min_max_update);
	QVBoxLayout* layout = new QVBoxLayout();

	if (arr != nullptr)
	{
		_arr = arr;
		_min_max_slider->setMinMax(_arr->minCoeff(), _arr->maxCoeff());

		data_struct::ArrayXr pts; 
		int w = arr->rows();
		int h = arr->cols();
		pts.resize(w * h);
		for (int i = 0; i < w; i++)
		{
			for (int j = 0; j < h; j++)
			{
				pts((i*w) + j) = ((*arr)(i, j));
			}
		}
		_historgram->updatePoints(pts);
	}

	QHBoxLayout* hbox = new QHBoxLayout();
	_btn_accept = new QPushButton("Accept");
	connect(_btn_accept, &QPushButton::pressed, this, &ContrastDialog::on_accepted);
	_btn_cancel = new QPushButton("Cancel");
	connect(_btn_cancel, &QPushButton::pressed, this, &ContrastDialog::on_rejected);
	hbox->addWidget(_btn_accept);
	hbox->addWidget(_btn_cancel);

	layout->addWidget(_historgram_widget);
	layout->addWidget(_min_max_slider);
	layout->addItem(hbox);
	setLayout(layout);

}

/*---------------------------------------------------------------------------*/

ContrastDialog::~ContrastDialog()
{

}

/*---------------------------------------------------------------------------*/

void ContrastDialog::min_max_update(int val)
{
	float minCoef = _min_max_slider->getUserMin();
	float maxCoef = _min_max_slider->getUserMax();
	emit on_min_max_update(minCoef, maxCoef);
}

/*---------------------------------------------------------------------------*/

void ContrastDialog::on_accepted()
{
	emit accepted();
	accept();
}

/*---------------------------------------------------------------------------*/

void ContrastDialog::on_rejected()
{
	emit rejected();
	close();
}

/*---------------------------------------------------------------------------*/