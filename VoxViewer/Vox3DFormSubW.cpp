#include "Vox3DForm.h"
#include "ui_Vox3DForm.h"

// void Vox3DForm::func_tableMaterialDefaultSet_matnameToColorCombo()
// //materialテーブルのセット。特定のマテリアルの場合、色設定コンボボックスを特定の色にセットする。
//{
//      QStringList matnameList, colorList;
//
//      //matnameList << "TEST01"	 << "TEST02"	 << "TEST03"	 << "TEST04"	 << "TEST05"	 << "TEST06"	 <<
//      "TEST07"	 << "TEST08"	 << "TEST09"	 << "TEST10"	 << "TEST11"	 << "TEST12"	 << "TEST13"	 <<
//      "TEST14"	 << "TEST15"	 << "TEST16"	 << "TEST17"	 << "TEST18"	 << "TEST19"	 << "TEST20"	 <<
//      "TEST21";
//      //colorList << "Lightcyan"	 << "Cyan"	 << "Blue"	 << "Navy"	 << "Olive"	 << "Greenyellow"	 << "Green"	 <<
//      "Yellow"	 << "Gold"	 << "Orange"	 << "Pink"	 << "Magenta"	 << "Red"	 << "MediumPurple"	 << "Purple"
//      << "Brown"	 << "White"	 << "Lightgray"	 << "DarkGray"	 << "Gray"	 << "Black";
//
//      matnameList << "AlCu" 	<< "Al" 	<< "CF-9th-B_Woollam" 	<< "CF-9th-G_Woollam" 	<< "CF-9th-R_Woollam" 	<<
//      "CF-Cinema-B_SCK" 	<< "CF-Cinema-G_SCK" 	<< "CF-Cinema-R_SCK" 	<< "CF-SIR_SCK" 	<<
//      "CF-SK9000C-Black_SCK" 	<< "CT_Woollam" 	<< "Cu" 	<< "LDR_TEOS_SCK" 	<< "LP-SiN_SCK" 	<< "LT-SiN_SCK"
//      << "LTO_SCK" 	<< "P-SiN_Woollam" 	<< "P-SiON_SCK" 	<< "Poly" 	<< "SCF-AH_SCK" 	<< "SCF-AO_SCK" 	<<
//      "SCF-PT_SCK" 	<< "SRO_SCK" 	<< "STSR_Woollam" 	<< "SiCN_SCK" 	<< "SiO2_SCK" 	<< "Si" 	<< "TaO_ATG"
//      << "Ta" 	<< "Ti" 	<< "UV-SiN_Woollam" 	<< "W" 	<< "CF-10th-B_SCK" 	<< "CF-10th-G_SCK" 	<<
//      "CF-6th-B_SCK" 	<< "CF-6th-G_SCK" 	<< "CF-6th-R_SCK" 	<< "CF-8kai-B_SCK" 	<< "CF-8kai-G_SCK" 	<<
//      "CF-8kai-R_SCK" 	<< "CF-Cyan_SCK" 	<< "CF-Real-B_SCK" 	<< "CF-Real-G_SCK" 	<< "CF-Real-R_SCK" 	<<
//      "M1C-PSIO_SCK" 	<< "TiN_ATG" 	<< "W_SCK" ; colorList << "Navy" 	<< "Navy" 	<< "Blue" 	<< "Green" 	<< "Red"
//      << "Blue" 	<< "Green" 	<< "Red" 	<< "Pink" 	<< "Black" 	<< "White" 	<< "Yellow" 	<< "Lightgray" 	<<
//      "Magenta" 	<< "Purple" 	<< "Gold" 	<< "Cyan" 	<< "Magenta" 	<< "Greenyellow" 	<< "Lightcyan" 	<<
//      "Pink" 	<< "White" 	<< "Olive" 	<< "Cyan" 	<< "Cyan" 	<< "Gray" 	<< "Black" 	<< "Purple" 	<< "Olive" 	<<
//      "Orange" 	<< "Pink" 	<< "Brown" 	<< "Blue" 	<< "Green" 	<< "Blue" 	<< "Green" 	<< "Red" 	<< "Blue" 	<<
//      "Green" 	<< "Red" 	<< "Lightcyan" 	<< "Blue" 	<< "Green" 	<< "Red" 	<< "Olive" 	<< "Purple" 	<<
//      "Brown" ;
//
//
//      QHash<QString, QString> nameToColorHash;
//      for(int i=0; i < matnameList.size(); i++){
//         nameToColorHash.insert(matnameList.at(i), colorList.at(i));
//      }
//
//      //foreach(QString key, nameToColorHash.keys()){ qDebug() <<
//      "[DEBUG]Vox3DFormSubW.cpp-func_tableMaterialDefaultSet_matnameToColorCombo() key=" << key << " value=" <<
//      nameToColorHash.value(key); }
//
//      for(int row=0; row < ui->tableWidget_material->rowCount(); row++){
//         QString matname = ui->tableWidget_material->item(row, 2)->text();
//         //qDebug() << "[DEBUG]Vox3DFormSubW.cpp-func_tableMaterialDefaultSet_matnameToColorCombo() matname=" <<
//         matname; if(nameToColorHash.contains(matname)){
//             //qDebug() << "[DEBUG]Vox3DFormSubW.cpp-func_tableMaterialDefaultSet_matnameToColorCombo() color=" <<
//             nameToColorHash.value(matname); CustomColorCombo *tmpcombo =
//             static_cast<CustomColorCombo*>(ui->tableWidget_material->cellWidget(row, 3));
//             tmpcombo->setCurrentText(nameToColorHash.value(matname));
//         }
//      }
// }
