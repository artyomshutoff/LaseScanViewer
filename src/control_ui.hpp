#pragma once
control::Match currentControl(){return controlDatabase.find(model.scan,model.label);}
std::optional<double> displayedVolume(){if(!comparison||comparison->options.metresPerUnit<=0)return {};return (cargo.calibrated?cargo.calibratedVolume:cargo.volume)*std::pow(comparison->options.metresPerUnit,3);}
std::vector<std::wstring> controlLines(){
 if(controlDatabase.path.empty()||model.points.empty())return {};auto match=currentControl();if(!match.record)return {L"Контроль: "+widen(match.reason)};
 const auto& r=*match.record;std::vector<std::wstring> lines{std::wstring(scanLayers.enabled&&scanLayers.tarp?L"TotalVolume (весь груз): ":L"TotalVolume: ")+num(r.total,3)+L" м³"};auto value=displayedVolume();
 if(value){double delta=*value-r.total;lines.push_back(L"Разница: "+std::wstring(delta>=0?L"+":L"")+num(delta,3)+L" м³");lines.push_back(r.total>0?L"Отклонение: "+std::wstring(delta>=0?L"+":L"")+num(delta/r.total*100,2)+L"%":L"Отклонение: % не определён (TotalVolume = 0)");}else lines.push_back(L"Расчёт ещё не выполнен / единицы не заданы");
 if(r.status!=5)lines.push_back(L"Запись не завершена · Status "+std::to_wstring(r.status));if(r.trailer>0)lines.push_back(L"TotalVolume включает прицеп");if(region.enabled)lines.push_back(L"Расчёт ограничен выбранной областью");return lines;
}
void drawControlComparison(){
 if(!textList)return;auto lines=controlLines();if(lines.empty())return;
 HFONT f=CreateFontW(-15,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,ANSI_CHARSET,0,0,NONANTIALIASED_QUALITY,0,L"Arial");auto old=SelectObject(glDC,f);
 std::vector<int> widths;int maxWidth=0;for(auto& line:lines){SIZE size{};GetTextExtentPoint32W(glDC,line.data(),int(line.size()),&size);while(size.cx>viewW-48&&line.size()>4){line.resize(line.size()-4);line+=L"...";GetTextExtentPoint32W(glDC,line.data(),int(line.size()),&size);}widths.push_back(size.cx);maxWidth=std::max(maxWidth,int(size.cx));}SelectObject(glDC,old);DeleteObject(f);
 glDisable(GL_DEPTH_TEST);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,viewW,0,viewH,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 glColor3f(.06f,.09f,.12f);int top=16+int(lines.size())*21;glBegin(GL_QUADS);glVertex2i(viewW-maxWidth-30,8);glVertex2i(viewW-8,8);glVertex2i(viewW-8,top+8);glVertex2i(viewW-maxWidth-30,top+8);glEnd();
 glColor3f(.76f,.85f,.82f);glListBase(textList);for(size_t i=0;i<lines.size();i++){glRasterPos2i(viewW-widths[i]-18,top-17-int(i)*21);glCallLists(GLsizei(lines[i].size()),GL_UNSIGNED_SHORT,lines[i].data());}
}
void loadControlDatabase(const std::filesystem::path& path){controlDatabase.load(path);status(L"База загружена · "+std::to_wstring(controlDatabase.ids.size())+L" записей · только сверка TotalVolume");redraw();}
void openControlDatabase(){if(busy)return;wchar_t file[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=mainWin;o.lpstrFilter=L"База измерений SQ3\0*.sq3;*.sqlite;*.db\0Все файлы\0*.*\0";o.lpstrFile=file;o.nMaxFile=32768;o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;if(GetOpenFileNameW(&o))loadControlDatabase(file);}
