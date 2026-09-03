//-----------------------------------------------------------------------------
// LayerManager.cpp : 图层管理实现
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "LayerManager.h"

//-----------------------------------------------------------------------------
// 从 acad.lin 加载线型(若当前图形中还没有)，返回线型对象 ID
//-----------------------------------------------------------------------------
static Acad::ErrorStatus LoadLinetype(const TCHAR* name, AcDbObjectId& outId) {
    AcDbDatabase* pDb = acdbHostApplicationServices()->workingDatabase();
    AcDbLinetypeTable* pTable = nullptr;
    Acad::ErrorStatus es = pDb->getSymbolTable(pTable, AcDb::kForWrite);
    if (es != Acad::eOk)
        return es;

    // 已存在则直接返回其 ID
    if (pTable->has(name)) {
        es = pTable->getAt(name, outId);
        pTable->close();
        return es;
    }

    // 定位 acad.lin 并从其中加载线型
    TCHAR path[MAX_PATH] = { 0 };
    if (acedFindFile(_T("acad.lin"), path, MAX_PATH) != RTNORM) {
        pTable->close();
        return Acad::eFileNotFound;
    }

    es = pDb->loadLineTypeFile(name, path);
    if (es == Acad::eOk)
        es = pTable->getAt(name, outId);
    pTable->close();
    return es;
}

//-----------------------------------------------------------------------------
// 确保图层存在
//-----------------------------------------------------------------------------
Acad::ErrorStatus EnsureLayer(const TCHAR* name, int colorIndex, const TCHAR* linetypeName,
                              AcDb::LineWeight lw) {
    AcDbDatabase* pDb = acdbHostApplicationServices()->workingDatabase();
    AcDbLayerTable* pTable = nullptr;
    Acad::ErrorStatus es = pDb->getSymbolTable(pTable, AcDb::kForWrite);
    if (es != Acad::eOk)
        return es;

    // 图层不存在时才创建
    if (!pTable->has(name)) {
        AcDbLayerTableRecord* pRec = new AcDbLayerTableRecord();
        pRec->setName(name);

        // 颜色
        AcCmColor c;
        c.setColorIndex((unsigned short)colorIndex);
        pRec->setColor(c);

        // 线宽
        pRec->setLineWeight(lw);

        // 线型（加载失败则保持默认 Continuous）
        if (linetypeName && _tcslen(linetypeName) > 0) {
            AcDbObjectId ltId;
            if (LoadLinetype(linetypeName, ltId) == Acad::eOk)
                pRec->setLinetypeObjectId(ltId);
        }

        AcDbObjectId id;
        es = pTable->add(id, pRec);
        pRec->close();
    }

    pTable->close();
    return es;
}
