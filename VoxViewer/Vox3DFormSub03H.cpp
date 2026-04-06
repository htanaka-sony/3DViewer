#include "Vox3DForm.h"
#include "ui_Vox3DForm.h"

#include "CommonSubFunc.h"

//-start- bkup 旧　関数自体 Sub01.cpp に移動する
// void Vox3DForm::func_tableMaterialDefaultSet_matnameToColorCombo()
// //materialテーブルのセット。特定のマテリアルの場合、色設定コンボボックスを特定の色にセットする。
//{
//     QStringList matnameList, colorList;
//
//     matnameList << "TEST01"	 << "TEST02"	 << "TEST03"	 << "TEST04"	 << "TEST05"	 << "TEST06"	 <<
//     "TEST07"	 << "TEST08"	 << "TEST09"	 << "TEST10"	 << "TEST11"	 << "TEST12"	 << "TEST13"	 <<
//     "TEST14"	 << "TEST15"	 << "TEST16"	 << "TEST17"	 << "TEST18"	 << "TEST19"	 << "TEST20"	 <<
//     "TEST21"; colorList << "Lightcyan"	 << "Cyan"	 << "Blue"	 << "Navy"	 << "Olive"	 << "Greenyellow"	 <<
//     "Green"	 << "Yellow"	 << "Gold"	 << "Orange"	 << "Pink"	 << "Magenta"	 << "Red"	 << "MediumPurple"
//     << "Purple"	 << "Brown"	 << "White"	 << "Lightgray"	 << "DarkGray"	 << "Gray"	 << "Black";
//
//     QHash<QString, QString> nameToColorHash;
//     for(int i=0; i < matnameList.size(); i++){
//        nameToColorHash.insert(matnameList.at(i), colorList.at(i));
//     }
//
//     for(int row=0; row < ui->tableWidget_material->rowCount(); row++){
//        QString matname = ui->tableWidget_material->item(row, 2)->text();
//        if(nameToColorHash.contains(matname)){
//            CustomColorCombo *tmpcombo = static_cast<CustomColorCombo*>(ui->tableWidget_material->cellWidget(row, 3));
//            tmpcombo->setCurrentText(nameToColorHash.value(matname));
//        }
//     }
//}
//-end- bkup 旧　関数自体 Sub01.cpp に移動する
