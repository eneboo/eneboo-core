// This may look like C code, but it's really -*- C++ -*-
/*
 * Copyright (C) 2008 Emweb bvba, Kessel-Lo, Belgium.
 *
 * See the LICENSE file for terms of use.
 */

#ifndef PAINT_EXAMPLE_H_
#define PAINT_EXAMPLE_H_

#include <Wt/WContainerWidget>

using namespace Wt;

class ShapesWidget;

class PaintExample : public WContainerWidget
{
public:
  PaintExample(WContainerWidget *root);

private:
  ShapesWidget *shapes_;

  void rotateShape(int v);
  void scaleShape(int v);
};

#endif // PAINT_EXAMPLE_H_