#ifndef STDAFX_H
#define STDAFX_H

#include <stdlib.h>
#include <QCoreApplication>
#include <QWheelEvent>
#include <QCloseEvent>
#include <QMessageBox>
#include <QDir>
#include <QQueue>
#include <stdio.h>
#include "opencv2/opencv.hpp"

#include "common.h"
#include "labeller.h"
#include "analyzecv/analyzecv.h"

#define IMMAINW 512
#define IMMAINH 512
#define IMPROJW 256
#define IMPROJH 256
#define WNDRESW (8 - 4)
#define WNDRESH (51 - 11)
#define TOOLMULT 3
#define MINMAINSZ 128

#endif // STDAFX_H
