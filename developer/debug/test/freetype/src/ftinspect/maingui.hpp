// maingui.hpp

// Copyright (C) 2016-2017 by Werner Lemberg.


#pragma once

#include "engine/engine.hpp"
#include "rendering/glyphbitmap.hpp"
#include "rendering/glyphoutline.hpp"
#include "rendering/glyphpointnumbers.hpp"
#include "rendering/glyphpoints.hpp"
#include "widgets/qcomboboxx.hpp"
#include "widgets/qgraphicsviewx.hpp"
#include "widgets/qpushbuttonx.hpp"
#include "widgets/qspinboxx.hpp"

#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileSystemWatcher>
#include <QGridLayout>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QPen>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalMapper>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QVariant>
#include <QVBoxLayout>

#include <ft2build.h>
#include FT_LCD_FILTER_H


class MainGUI
: public QMainWindow
{
  Q_OBJECT

public:
  MainGUI();
  ~MainGUI();

  void setDefaults();
  void update(Engine*);

  friend class Engine;
  friend FT_Error faceRequester(FTC_FaceID,
                                FT_Library,
                                FT_Pointer,
                                FT_Face*);

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void about();
  void aboutQt();
  void adjustGlyphIndex(int);
  void checkAntiAliasing();
  void checkAutoHinting();
  void checkCurrentFaceIndex();
  void checkCurrentFontIndex();
  void checkCurrentNamedInstanceIndex();
  void checkHinting();
  void checkHintingMode();
  void checkLcdFilter();
  void checkShowPoints();
  void checkUnits();
  void closeFont();
  void drawGlyph();
  void loadFonts();
  void nextFace();
  void nextFont();
  void nextNamedInstance();
  void previousFace();
  void previousFont();
  void previousNamedInstance();
  void watchCurrentFont();
  void zoom();

private:
  Engine* engine;

  QStringList fontList;
  int currentFontIndex;

  long currentNumberOfFaces;
  long currentFaceIndex;

  int currentNumberOfNamedInstances;
  int currentNamedInstanceIndex;

  int currentNumberOfGlyphs;
  int currentGlyphIndex;

  int currentCFFHintingMode;
  int currentTTInterpreterVersion;

  // layout related stuff
  GlyphOutline *currentGlyphOutlineItem;
  GlyphPoints *currentGlyphPointsItem;
  GlyphPointNumbers *currentGlyphPointNumbersItem;
  GlyphBitmap *currentGlyphBitmapItem;

  QAction *aboutAct;
  QAction *aboutQtAct;
  QAction *closeFontAct;
  QAction *exitAct;
  QAction *loadFontsAct;

  QCheckBox *autoHintingCheckBox;
  QCheckBox *blueZoneHintingCheckBox;
  QCheckBox *hintingCheckBox;
  QCheckBox *horizontalHintingCheckBox;
  QCheckBox *segmentDrawingCheckBox;
  QCheckBox *showBitmapCheckBox;
  QCheckBox *showOutlinesCheckBox;
  QCheckBox *showPointNumbersCheckBox;
  QCheckBox *showPointsCheckBox;
  QCheckBox *verticalHintingCheckBox;
  QCheckBox *warpingCheckBox;

  QComboBoxx *antiAliasingComboBoxx;
  QComboBoxx *hintingModeComboBoxx;
  QComboBox *lcdFilterComboBox;
  QComboBox *unitsComboBox;

  QDoubleSpinBox *sizeDoubleSpinBox;

  QFileSystemWatcher *fontWatcher;

  QGraphicsScene *glyphScene;
  QGraphicsViewx *glyphView;

  QGridLayout *fontLayout;
  QGridLayout *infoRightLayout;

  QHash<int, int> hintingModesTrueTypeHash;
  QHash<int, int> hintingModesCFFHash;
  QHash<FT_LcdFilter, int> lcdFilterHash;

  QHBoxLayout *antiAliasingLayout;
  QHBoxLayout *blueZoneHintingLayout;
  QHBoxLayout *ftinspectLayout;
  QHBoxLayout *gammaLayout;
  QHBoxLayout *hintingModeLayout;
  QHBoxLayout *horizontalHintingLayout;
  QHBoxLayout *infoLeftLayout;
  QHBoxLayout *lcdFilterLayout;
  QHBoxLayout *navigationLayout;
  QHBoxLayout *pointNumbersLayout;
  QHBoxLayout *segmentDrawingLayout;
  QHBoxLayout *sizeLayout;
  QHBoxLayout *verticalHintingLayout;
  QHBoxLayout *warpingLayout;

  QLabel *antiAliasingLabel;
  QLabel *dpiLabel;
  QLabel *fontFilenameLabel;
  QLabel *fontNameLabel;
  QLabel *gammaLabel;
  QLabel *glyphIndexLabel;
  QLabel *glyphNameLabel;
  QLabel *hintingModeLabel;
  QLabel *lcdFilterLabel;
  QLabel *sizeLabel;
  QLabel *zoomLabel;

  QList<int> hintingModesAlwaysDisabled;

  QLocale *locale;

  QMenu *menuFile;
  QMenu *menuHelp;

  QPen axisPen;
  QPen blueZonePen;
  QPen gridPen;
  QPen offPen;
  QPen onPen;
  QPen outlinePen;
  QPen segmentPen;

  QPushButton *nextFaceButton;
  QPushButton *nextFontButton;
  QPushButton *nextNamedInstanceButton;
  QPushButton *previousFaceButton;
  QPushButton *previousFontButton;
  QPushButton *previousNamedInstanceButton;

  QPushButtonx *toEndButtonx;
  QPushButtonx *toM1000Buttonx;
  QPushButtonx *toM100Buttonx;
  QPushButtonx *toM10Buttonx;
  QPushButtonx *toM1Buttonx;
  QPushButtonx *toP1000Buttonx;
  QPushButtonx *toP100Buttonx;
  QPushButtonx *toP10Buttonx;
  QPushButtonx *toP1Buttonx;
  QPushButtonx *toStartButtonx;

  QSignalMapper *glyphNavigationMapper;

  QSlider *gammaSlider;

  QSpinBox *dpiSpinBox;
  QSpinBoxx *zoomSpinBox;

  QTabWidget *tabWidget;

  QTimer *timer;

  QVBoxLayout *generalTabLayout;
  QVBoxLayout *leftLayout;
  QVBoxLayout *rightLayout;

  QVector<QRgb> grayColorTable;
  QVector<QRgb> monoColorTable;

  QWidget *ftinspectWidget;
  QWidget *generalTabWidget;
  QWidget *leftWidget;
  QWidget *rightWidget;
  QWidget *mmgxTabWidget;

  enum AntiAliasing
  {
    AntiAliasing_None,
    AntiAliasing_Normal,
    AntiAliasing_Light,
    AntiAliasing_LCD,
    AntiAliasing_LCD_BGR,
    AntiAliasing_LCD_Vertical,
    AntiAliasing_LCD_Vertical_BGR
  };
  enum HintingMode
  {
    HintingMode_TrueType_v35,
    HintingMode_TrueType_v38,
    HintingMode_TrueType_v40,
    HintingMode_CFF_FreeType,
    HintingMode_CFF_Adobe
  };
  enum LCDFilter
  {
    LCDFilter_Default,
    LCDFilter_Light,
    LCDFilter_None,
    LCDFilter_Legacy
  };
  enum Units
  {
    Units_px,
    Units_pt
  };

  void createActions();
  void createConnections();
  void createLayout();
  void createMenus();
  void clearStatusBar();
  void createStatusBar();
  void readSettings();
  void setGraphicsDefaults();
  void showFont();
  void writeSettings();
};


// end of maingui.hpp
