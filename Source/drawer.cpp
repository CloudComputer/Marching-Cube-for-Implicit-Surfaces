#define GLM_FORCE_RADIANS
#include "drawer.h"
#include "imgui_impl_glut.h" 
#include <algorithm>
#include <memory> 
#include <thread>
#include <process.h> 
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "ShaderLib.h"
#include "vao.h"
#include "normal.h"
using namespace std;

/* Window information */
int nPolynomialHeight = 70;
int nBarWidth = 280;
int nBase;
int nWindowHeight;
int nWindowWidth;
int windowID = -1;
float nColor = 48;
/* Data information */
const Poly_Data* pData;
/* Shader information */
static const std::string strMeshVertexShader("shader\\mesh_vs.glsl");
static const std::string strMeshFragmentShader("shader\\mesh_fs.glsl");
static const std::string strMeshGeometryShader("shader\\mesh_gs.glsl");
static const std::string strCubeVertexShader("shader\\cube_vs.glsl");
static const std::string strCubeFragmentShader("shader\\cube_fs.glsl");

/* Parameter information */
static char szEquation[256] = "(x^2+y^2-1)^2 + (x^2+z^2-1)^2 + (z^2+y^2-1)^2 - 0.5";
static float fGrid = 0.2f;
static float fSurfaceConstant = 0.0f;
static float fStepDistance = 0.5f;
static float fScaler[3] = { 1.1f, 1.1f, 1.1f };
static float fSeeding[3] = {0.8f,0.8f,0.9f};
static int nMovieSpeed = 5;
static bool bStepMode,bSeedingMode, bTranslucent, bInvertFace, bRepeatingMode;
static ImVec4 colBackground = ImColor(1.0f, 1.0f,1.0f, 1.0f);
static ImVec4 colModelFrontFace = ImColor(1.0f, 0.3f, 0.3f, 1.0f);
static ImVec4 colModelBackFace = ImColor(0.5f, 0.5f, 0.5f, 1.0f);
static ImVec4 colCubeSurface = ImColor(0.7f, 0.7f, 0.7f, 0.5f);
static ImVec4 colCubeEdge = ImColor(0.33f, 0.33f, 0.33f, 1.0f);
static ImVec4 colModelEdge = ImColor(0.0f, 0.0f,0.0f, 1.0f);
static ImVec4 colIntersectPoint = ImColor(0.0f, 0.0f, 1.0f, 1.0f);
static ImVec4 colIntersectSurface = ImColor(0.0f, 1.0f, 1.0f, 1.0f);
static ImVec4 colIntersectSurfaceEdges = ImColor(0.0f, 0.0f, 1.0f, 1.0f);
static ImVec4 colInCornerPoint = ImColor(0.0f, 1.0f, 0.0f, 1.0f);
static ImVec4 colOutCornerPoint = ImColor(1.0f, 0.0f, 0.0f, 1.0f);
string op[4] = { ">=", "<=", "<", ">" };
bool bLeftPressed, bRightPressed, bHasInit, bMovieMode,bPlaying, bPause, bCubeStep,bReset;
bool bEnterPressed = false;
int nLastX, nLastY, nCurX, nCurY;
int nCubeStep;
int nLineWidth = 3;
GLuint vao[3], vbo[3], ibo[3], normal[3];//0 for model, 1 for cube, 2 for intersect
glm::mat4 M,V,P, Viewport;
vector<glm::vec3> vNormal;
vector<float> vIntersectVertex, vIntersectIndex;
//combo
static char szlhs1[256], szlhs2[256], szlhs3[256];
static bool bConstraint1, bConstraint2, bConstraint3;
static float frhs1, frhs2, frhs3;
static int item1, item2, item3;
// objects 
Drawer* pDrawer = nullptr;
ShaderLib MeshShader,CubeShader;
thread movie;

//cube idx
unsigned int idxCube[36] = { 1, 0, 2, 2, 0, 3, 0, 5, 4, 5, 0, 1, 1, 6, 5, 6, 1, 2, 2, 7, 6, 7, 2, 3,3, 0, 7, 0, 4, 7, 4, 5, 6, 6, 7, 4 };
unsigned int idxEdge[30] = { 0, 1, 2, 3, 0, 1, 5, 6, 2, 1, 5, 4, 7, 6, 5, 4, 0, 3, 7, 4, 4, 5, 1, 0, 4, 7, 3, 2, 6, 7 };

void BufferData(GLuint ibo, GLuint ni, void* pi, GLuint vbo, GLuint nv, void* pv) {
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, nv, pv, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni, pi, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
void BufferData(GLuint ibo, GLuint ni, void* pi, GLuint vbo, GLuint nv, void* pv, GLuint normal, GLuint nn, void* pn){
// 	glNamedBufferData(ibo, ni, pi, GL_DYNAMIC_DRAW);
// 	glNamedBufferData(vbo, nv, pv, GL_DYNAMIC_DRAW);	
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, nv, pv, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, normal);
	glBufferData(GL_ARRAY_BUFFER, nn, pn, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni, pi, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
void SetStepData(){
	if (pData == nullptr) return;

	//model
	if (!pData->tri_list.empty() && !pData->vertex_list.empty()){
		BufferData(ibo[0], pData->tri_list.size()*sizeof(unsigned int), (void*)&pData->tri_list[0],
					vbo[0], pData->vertex_list.size()*sizeof(float), (void*)&pData->vertex_list[0],
					normal[0], vNormal.size()*sizeof(glm::vec3), (void*)&vNormal[0]);
	}

	//cube
	BufferData(0, 0, 0, vbo[1], pData->step_data.corner_coords.size()*sizeof(float), (void*)&pData->step_data.corner_coords[0]);

	//intersect	
	int num = pData->step_data.intersect_coord.size();
	if (num > 0){
		BufferData(0, 0, 0, vbo[2], pData->step_data.intersect_coord.size()*sizeof(float), (void*)&pData->step_data.intersect_coord[0]);
	}
}

static int Movie()
{		
	if (pDrawer == nullptr) return false;
	if (pData == nullptr) return false;
	float fTotalStep = (float)pData->step_data.step_i;
	
	for (int i = 0; bPlaying && i != pData->step_data.step_i;){
		if (bPause) {
			nCubeStep = 3;
			continue;
		}
		nCubeStep = -1;		
		pDrawer->Recalculate();	
		
		if (pData->step_data.intersect_coord.size()>0)
		for (int nStep = 0; nStep < 4; nStep++)
		{
			nCubeStep++;
			Sleep((10 - nMovieSpeed) * 20);
		}		
		Sleep((10 - nMovieSpeed) * 50);
	}	
	pDrawer->SetStepMode(false);
	pDrawer->ResetStep();
	bStepMode = false;
	bPlaying = false;			
	_endthread();	
	return true;
}

void DrawGUI()
{	
	if (pDrawer == nullptr) return;
	ImGui_ImplGlut_NewFrame("Marching Cube",0.5f);
	ImVec2 wsize(nBarWidth, nWindowHeight - nPolynomialHeight);
	ImGui::SetWindowFontScale(1.5);
	ImGui::SetWindowSize(wsize);
	ImVec2 wpos(nWindowWidth - nBarWidth, nPolynomialHeight);
	ImGui::SetWindowPos(wpos);
	
	if (!bMovieMode && (ImGui::Button("Refresh") || bEnterPressed )){
		bEnterPressed = false;
		if (!bStepMode) {
			if (!pDrawer->SetEquation(string(szEquation))) {
				ImGui::OpenPopup("Equation error");
			}
			else {
				pDrawer->SetGridSize(fGrid);
				pDrawer->Recalculate();
				if (pData && !pData->tri_list.empty() && !pData->vertex_list.empty())
					BufferData(ibo[0], pData->tri_list.size() * sizeof(unsigned int), (void*)&pData->tri_list[0],
						vbo[0], pData->vertex_list.size() * sizeof(float), (void*)&pData->vertex_list[0],
						normal[0], vNormal.size() * sizeof(glm::vec3), (void*)&vNormal[0]);
			}
		}
		else{
			if (!pDrawer->SetEquation(string(szEquation))) {
				ImGui::OpenPopup("Equation error");
			}
			else {
				if (bCubeStep) {
					nCubeStep++;
					if (nCubeStep == 4) {
						nCubeStep = 0;
						pDrawer->Recalculate();
					}
				}
				else pDrawer->Recalculate();
				pDrawer->GetPolyData();
				if (pData != nullptr) {
					bCubeStep = pData->step_data.intersect_coord.size() > 0 ? true : false;
				}
				bReset = false;
			}
		}
	}ImGui::SameLine();
	if (!bMovieMode && ImGui::Button("Reset")){		
		pDrawer->ResetStep();
		CubeShader.use();
		for (int i = 0; i < 3; i++)
			BufferData(ibo[i], 0, 0, vbo[i], 0, 0);		
		pDrawer->ResetAlldata();	
		bReset = true;
	}
	ImGui::Separator();
	if (!bMovieMode && ImGui::SliderFloat("Grid size", &fGrid, 0.05f, 0.5f,"%.2f")){		
		pDrawer->SetGridSize(fGrid);
		pDrawer->ResetStep();					
	}
	if (!bMovieMode && ImGui::Checkbox("Translucent", &bTranslucent)){
		if (bTranslucent){
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			colModelFrontFace.w = 0.5f;
			nLineWidth = 1;
		}
		else{
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			colModelFrontFace.w = 1.0f;
			nLineWidth = 3;
		}
		MeshShader.use();
		MeshShader.setUniform("uTranslucent", bTranslucent);
	}
	
	if (!bMovieMode)ImGui::Separator();
	if (!bMovieMode && ImGui::Button("Movie mode")){
		if (bMovieMode){
			bMovieMode = false;
			bPause = false;
		}
		else{
			bStepMode = true;
			bMovieMode = true;
			bPause = false;
		}			
	}
	if (bMovieMode && ImGui::Button("Play")){
		bPause = false;		
		bStepMode = true;		
		if (!bPlaying) {
			pDrawer->SetStepMode(true);
			if (!pDrawer->SetEquation(string(szEquation))) {
				ImGui::OpenPopup("Equation error");
			}
			else {
				pDrawer->SetGridSize(fGrid);
				bPlaying = true;
				bReset = false;
				movie = thread(Movie);
			}
		}
	}
	if (bMovieMode)ImGui::SameLine();
	if (bMovieMode && ImGui::Button("Pause")){
		if(bPlaying) bPause = true;
	}
	if (bMovieMode)ImGui::SameLine();
	if (bMovieMode && ImGui::Button("Reset")){		
		if (bPlaying || bPause) {
			bPlaying = false;
			bPause = false;
			movie.join();
		}
		pDrawer->ResetStep();
		for (int i = 0; i < 3; i++)
			BufferData(ibo[i], 0, 0, vbo[i], 0, 0);
		pDrawer->ResetAlldata();
		bReset = true;
	}
	
	if (bMovieMode)ImGui::SliderInt("Movie speed", &nMovieSpeed, 1, 9);		
	if (bMovieMode && ImGui::Checkbox("Translucent", &bTranslucent)){
		if (bTranslucent){
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			colModelFrontFace.w = 0.5f;
			nLineWidth = 1;
		}
		else{
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			colModelFrontFace.w = 1.0f;
			nLineWidth = 3;
		}
		MeshShader.use();
		MeshShader.setUniform("uTranslucent", bTranslucent);
	}
	if (!bMovieMode && ImGui::Checkbox("Step mode", &bStepMode)){
		if (!pDrawer->SetEquation(string(szEquation))) {
			ImGui::OpenPopup("Equation error");
		}
		else {
			for (int i = 0; i < 3; i++)
				BufferData(ibo[i], 0, 0, vbo[i], 0, 0);
			pDrawer->ResetStep();
			pDrawer->ResetAlldata();
			pDrawer->SetStepMode(bStepMode);
		}
	}
	if (!bSeedingMode && bStepMode){
		char ch[20] = { 0 };
		if (pData && bStepMode && pData->step_data.step_i >= 0)
			sprintf_s(ch, "RemainSteps:%d", pData->step_data.step_i);
		ImGui::Text(ch);
	}
	if (!bMovieMode)ImGui::Separator();
	if (!bMovieMode && ImGui::Checkbox("Seeding mode", &bSeedingMode)){				
		pDrawer->SetSeedMode(bSeedingMode);
		pDrawer->SetSeed(fSeeding);
	}	
	if (!bMovieMode && bSeedingMode && ImGui::InputFloat3("Seeding point", fSeeding, 2))
		pDrawer->SetSeed(fSeeding);

	//Constraint1
	if (!bMovieMode && ImGui::Checkbox("Constraint 1", &bConstraint1)){
		pDrawer->UseExtraConstraint0(bConstraint1);
	}
	if (!bMovieMode && bConstraint1 && ImGui::InputText("Equation 1", szlhs1, 256, 0))
		pDrawer->SetConstraint0(szlhs1, op[item1], frhs1);
	if (!bMovieMode && bConstraint1 && ImGui::Combo("Operation 1", &item1, ">=\0<=\0<\0>\0"))
		pDrawer->SetConstraint0(szlhs1, op[item1], frhs1);
	if (!bMovieMode && bConstraint1 && ImGui::InputFloat("Const 1", &frhs1, 0.0f, 0.0f, 2))
		pDrawer->SetConstraint0(szlhs1, op[item1], frhs1);
	
	//Constraint2
	if (!bMovieMode && ImGui::Checkbox("Constraint 2", &bConstraint2)) {
		pDrawer->UseExtraConstraint1(bConstraint2);
	}
	if (!bMovieMode && bConstraint2 && ImGui::InputText("Equation 2", szlhs2, 256, 0))
		pDrawer->SetConstraint1(szlhs2, op[item2], frhs2);
	if (!bMovieMode && bConstraint2 && ImGui::Combo("Operation 2", &item2, ">=\0<=\0<\0>\0"))
		pDrawer->SetConstraint1(szlhs2, op[item2], frhs2);
	if (!bMovieMode && bConstraint2 && ImGui::InputFloat("Const 2", &frhs2, 0.0f, 0.0f, 2))
		pDrawer->SetConstraint1(szlhs2, op[item2], frhs2);

	//Constraint3
	if (!bMovieMode && ImGui::Checkbox("Constraint 3", &bConstraint3)) {
		pDrawer->UseExtraConstraint2(bConstraint3);
	}
	if (!bMovieMode && bConstraint3 && ImGui::InputText("Equation 3", szlhs3, 256, 0))
		pDrawer->SetConstraint2(szlhs3, op[item3], frhs3);
	if (!bMovieMode && bConstraint3 && ImGui::Combo("Operation 3", &item3, ">=\0<=\0<\0>\0"))
		pDrawer->SetConstraint2(szlhs3, op[item3], frhs3);
	if (!bMovieMode && bConstraint3 && ImGui::InputFloat("Const 3", &frhs3, 0.0f, 0.0f, 2))
		pDrawer->SetConstraint2(szlhs3, op[item3], frhs3);

	ImGui::Separator();
	if (ImGui::Button("Save Mesh")){
		if (pData == nullptr || pData && pData->vertex_list.size() == 0)
			ImGui::OpenPopup("Save error");
		else{
			if (pDrawer)pDrawer->SaveFile();
		}
	}
	if (ImGui::BeginPopupModal("Save error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("No data to save.\n\n");
		if (ImGui::Button("OK", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Mesh")){
		pDrawer->LoadFile();
		pDrawer->ResetStep();		
		vector<glm::vec3> vLoadNormal = CalculateNormal(pData);
		if (pData && !pData->tri_list.empty() && !pData->vertex_list.empty())
			BufferData(ibo[0], pData->tri_list.size()*sizeof(unsigned int), (void*)&pData->tri_list[0],
				vbo[0], pData->vertex_list.size()*sizeof(float), (void*)&pData->vertex_list[0],
				normal[0], vLoadNormal.size()*sizeof(glm::vec3), (void*)&vLoadNormal[0]);
		else ImGui::OpenPopup("Load error");
	}
	if (ImGui::BeginPopupModal("Load error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("No data to load.\n\n");
		if (ImGui::Button("OK", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	if (ImGui::BeginPopupModal("Equation error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Invalid Equation.\n\n");
		if (ImGui::Button("OK", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	if (bMovieMode)ImGui::Separator();
	if (bMovieMode && ImGui::Button("Exit movie mode")){
		bMovieMode = false;		
		bStepMode = false; 
		if (bPlaying || bPause) {
			bPlaying = false;
			bPause = false;
			movie.join();
		}			
		pDrawer->ResetAlldata();
	}
	if (bRightPressed)
		ImGui::OpenPopup("menu");
	if (ImGui::BeginPopup("menu"))
	{
		if (ImGui::ColorPlate(&colBackground, "Background color")){
			glClearColor(colBackground.x, colBackground.y, colBackground.z, 1.0f);
		}
		ImGui::ColorPlate(&colModelFrontFace, "Surface front color");
		ImGui::ColorPlate(&colModelBackFace, "Surface back color");
		ImGui::ColorPlate(&colCubeEdge, "Cube edge color");
		ImGui::ColorPlate(&colModelEdge, "Surface edge color");
		ImGui::ColorPlate(&colInCornerPoint, "Inside point color");
		ImGui::ColorPlate(&colOutCornerPoint, "Outside point color");
		ImGui::ColorPlate(&colIntersectPoint, "Intersect point color");
		ImGui::ColorPlate(&colIntersectSurface, "Intersect surface color");
		ImGui::ColorPlate(&colIntersectSurfaceEdges, "Intersect edges color");		
		ImGui::EndPopup();
	}
	
	//top bar	
	ImGui::Begin("ImGui Demo", 0.55f, false, ImVec2(nWindowHeight, 0), -1.0f, ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	ImVec2 psize(nWindowWidth, nPolynomialHeight);	
	ImGui::SetWindowSize(psize);
	ImVec2 ppos(0,0);
	ImGui::SetWindowPos(ppos);
	ImGui::Text("P(ax,by,cz)="); ImGui::SameLine();
	ImGui::SetWindowFontScale(1.5);
	ImGui::InputText("=", szEquation, 256, 0);ImGui::SameLine();	
	if (ImGui::InputFloat("s", &fSurfaceConstant, 0.0f, 0.0f, 2)){
		pDrawer->SetSurfaceConstant(fSurfaceConstant);
		pDrawer->ResetStep();		
	}
	ImGui::Text("(a,b,c) = "); ImGui::SameLine();
	if (ImGui::InputFloat3(" ", fScaler)){		
		pDrawer->SetScaling(fScaler);
	}	
	ImGui::SameLine();
	if (!bMovieMode && ImGui::Button("Load Equation")){
		string str;
		if (pDrawer->LoadEquation(str)){
			for (int i = 0; i < str.length(); i++)
				szEquation[i] = str.c_str()[i];
			szEquation[str.length()] = '\0';
		}		
	}ImGui::SameLine();
	if (!bMovieMode && ImGui::Button("Save Equation")){
		pDrawer->SaveEquation();
	}
	
	ImGui::End();	
	ImGui::Render();
	bHasInit = true;	
}
glm::vec3 GetArcballVector(int x, int y) {
	glm::vec3 P = glm::vec3(1.0*x / nWindowWidth * 2 - 1.0,
		1.0*y / nWindowHeight * 2 - 1.0,0);//屏幕坐标系变为[-1,1]
	P.y = -P.y;
	float OP_squared = P.x * P.x + P.y * P.y;
	if (OP_squared <= 1 * 1)
		P.z = sqrt(1 * 1 - OP_squared);  // Pythagore
	else
		P = glm::normalize(P);  // nearest point
	return P;
}

void Arcball() {	
	// Handle 
	if (nCurX != nLastX || nCurY != nLastY) {
		glm::vec3 va = GetArcballVector(nLastX, nLastY);
		glm::vec3 vb = GetArcballVector(nCurX, nCurY);
		float angle = acos(min(1.0f, glm::dot(va, vb)));
		glm::vec3 axis_in_camera_coord = glm::cross(va, vb);
		glm::mat3 camera2object = glm::inverse(glm::mat3(V) * glm::mat3(M));
		glm::vec3 axis_in_object_coord = camera2object * axis_in_camera_coord;
		M = glm::rotate(M, angle, axis_in_object_coord);
		nLastX = nCurX;
		nLastY = nCurY;
	}

	// Model
	// call in main_object.draw() - main_object.M
	MeshShader.use();
	mat4 mv = V*M;
	MeshShader.setUniform("NormalMatrix", mat3(vec3(mv[0]), vec3(mv[1]), vec3(mv[2])));
	MeshShader.setUniform("ViewportMatrix", Viewport);
 	MeshShader.setUniform("M", M);
	MeshShader.setUniform("V", V); 
 	MeshShader.setUniform("P", P);
	CubeShader.use();
	CubeShader.setUniform("M", M);
	CubeShader.setUniform("V", V);
	CubeShader.setUniform("P", P);		
}
void DrawCube(){
	if (!bStepMode) return;	
	if (pData == nullptr) return;
	if (pData->step_data.corner_coords.empty()) return;		
	if (bReset) return;

	SetStepData();	
	CubeShader.use();
	//cube	
	glEnable(GL_BLEND);
	glDepthMask(false);	
	BufferData(ibo[1], sizeof(idxCube), idxCube, 0, 0, 0);//修改数据后要bind
	CubeShader.setUniform("uFrontColor", glm::vec4(colCubeSurface.x, colCubeSurface.y, colCubeSurface.z, 0.5f));
	CubeShader.setUniform("uBackColor", glm::vec4(colCubeSurface.x, colCubeSurface.y, colCubeSurface.z, 0.5f));
	glBindVertexArray(vao[1]);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);	
	glBindVertexArray(0);
	glDepthMask(true);
 	if (!bTranslucent)
 		glDisable(GL_BLEND);	

	//edge	
	BufferData(ibo[1], sizeof(idxEdge), idxEdge, 0, 0, 0);
	CubeShader.setUniform("uFrontColor", glm::vec4(colCubeEdge.x, colCubeEdge.y, colCubeEdge.z, 1.0f));
	glBindVertexArray(vao[1]);	
	glDrawElements(GL_LINE_STRIP, 30, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	//corner,1	
	glDepthMask(false);
	if (nCubeStep > 0){
		int n = 0;
		for (auto p : pData->step_data.corner_values){
			glm::vec4 color = p < fSurfaceConstant ? glm::vec4(colInCornerPoint.x, colInCornerPoint.y, colInCornerPoint.z, 1.0f)
				: glm::vec4(colOutCornerPoint.x, colOutCornerPoint.y, colOutCornerPoint.z, 1.0f);
			CubeShader.setUniform("uFrontColor", color);
			glBindVertexArray(vao[1]);
			glDrawArrays(GL_POINTS, n++, 1);
			glBindVertexArray(0);
		}
	}
	//intersect	point,2
	if (nCubeStep > 1){		
		CubeShader.setUniform("uFrontColor", glm::vec4(colIntersectPoint.x, colIntersectPoint.y, colIntersectPoint.z, 1.0f));
		glBindVertexArray(vao[2]);
		glDrawArrays(GL_POINTS, 0, pData->step_data.intersect_coord.size() / 3);
		glBindVertexArray(0);
	}
	glDepthMask(true);
	
	//polygon,3	
	if (pData->step_data.tri_vlist.size() > 0){	
		BufferData(ibo[2], pData->step_data.tri_vlist.size()*sizeof(unsigned int), (void*)&pData->step_data.tri_vlist[0], 0, 0, 0);//修改数据后要bind
		glBindVertexArray(vao[2]);
		if (nCubeStep > 2){
			CubeShader.setUniform("uFrontColor", glm::vec4(colIntersectSurface.x, colIntersectSurface.y, colIntersectSurface.z, 1.0f));
			CubeShader.setUniform("uBackColor", glm::vec4(colIntersectSurface.x, colIntersectSurface.y, colIntersectSurface.z, colModelBackFace.w));
			glDrawElements(GL_TRIANGLES, pData->step_data.intersect_coord.size(), GL_UNSIGNED_INT, 0);
			//polygon edges
			glLineWidth(4);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			CubeShader.setUniform("uFrontColor", glm::vec4(colIntersectSurfaceEdges.x, colIntersectSurfaceEdges.y, colIntersectSurfaceEdges.z, 1.0f));
			CubeShader.setUniform("uBackColor", glm::vec4(colIntersectSurfaceEdges.x, colIntersectSurfaceEdges.y, colIntersectSurfaceEdges.z, 1.0f));
			glDrawElements(GL_TRIANGLES, pData->step_data.intersect_coord.size(), GL_UNSIGNED_INT, 0);
			glLineWidth(2);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		glBindVertexArray(0);
	}	
}	
	

void DrawModel(){		
	if(pData == nullptr) return;	
	if(pData->tri_list.size()==0) return;	
	
	MeshShader.use();
	MeshShader.setUniform("uLineColor", glm::vec4(colModelEdge.x, colModelEdge.y, colModelEdge.z, 1.0f));
	MeshShader.setUniform("uFrontColor", glm::vec4(colModelFrontFace.x, colModelFrontFace.y, colModelFrontFace.z, bTranslucent ? 0.5f : 1.0f));
	MeshShader.setUniform("uBackColor", glm::vec4(colModelBackFace.x, colModelBackFace.y, colModelBackFace.z, colModelBackFace.w));
	glBindVertexArray(vao[0]);
	glDrawElements(GL_TRIANGLES, pData->tri_list.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);	
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	Arcball();
	DrawModel();
	DrawCube();	
	DrawGUI();	
	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y){
	ImGui_ImplGlut_KeyCallback(key);	
	if (pDrawer && key == '\r'){
		bEnterPressed = true;
/*		if (!pDrawer->SetEquation(string(szEquation))) {
			ImGui::OpenPopup("Equation error");
		}
		pDrawer->SetGridSize(fGrid);
		pDrawer->Recalculate();		
		vector<glm::vec3> vNormal = CalculateNormal(pData);
		if (pData && !pData->tri_list.empty() && !pData->vertex_list.empty())
			BufferData(ibo[0], pData->tri_list.size()*sizeof(unsigned int), (void*)&pData->tri_list[0],
				vbo[0], pData->vertex_list.size()*sizeof(float), (void*)&pData->vertex_list[0],
				normal[0], vNormal.size()*sizeof(glm::vec3), (void*)&vNormal[0]);
	*/
	}
}

void keyboard_up(unsigned char key, int x, int y){
	ImGui_ImplGlut_KeyUpCallback(key);
}

void special_up(int key, int x, int y){
	ImGui_ImplGlut_SpecialUpCallback(key);
}

void passive(int x, int y){
	ImGui_ImplGlut_PassiveMouseMotionCallback(x, y);
}
void special(int key, int x, int y){
	ImGui_ImplGlut_SpecialCallback(key);
	if (bStepMode == false) return;
	switch (key)
	{
	case GLUT_KEY_RIGHT:		
		if (pDrawer) {
			if (bCubeStep){
				nCubeStep++;
				bReset = false;
				if (nCubeStep == 4) {
					nCubeStep = 0;
					pDrawer->Recalculate();					
				}
				else break;
			}			
			else {
				pDrawer->Recalculate();	
				bReset = false;
			}			
			if (pData)
				bCubeStep = pData->step_data.intersect_coord.size() > 0 ? true : false;					
		}			
		break;
	}
}

void motion(int x, int y){
	ImGui_ImplGlut_MouseMotionCallback(x, y);
	if (bLeftPressed && x < nWindowWidth - nBarWidth && y>nPolynomialHeight)
	 {  // if left button is pressed
		nCurX = x;
		nCurY = y;
	}
}

void mouse(int button, int state, int x, int y)
{
	ImGui_ImplGlut_MouseButtonCallback(button, state);
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		bLeftPressed = true;
		if (x < nWindowWidth - nBarWidth){
			nLastX = nCurX = x;
			nLastY = nCurY = y;
		}
	}
	else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN){
		bRightPressed = true;
	}
	else {
		bLeftPressed = false;
		bRightPressed = false;
	}
}

void idle(){
	glutPostRedisplay();
}
void reshape(int w, int h){	
	int nMaxLenth = h - nPolynomialHeight;
	glViewport((w - nBarWidth - nMaxLenth) / 2.0f, 0, nMaxLenth, nMaxLenth);
	nWindowWidth = w;
	nWindowHeight = h;
	float w2 = nMaxLenth / 2.0f;
	float h2 = nMaxLenth / 2.0f;
	Viewport = mat4(vec4(w2, 0.0f, 0.0f, 0.0f),
		vec4(0.0f, h2, 0.0f, 0.0f),
		vec4(0.0f, 0.0f, 1.0f, 0.0f),
		vec4(w2, h2 , 0.0f, 1.0f));
}

void CompileAndLinkShader(){
	try {
		MeshShader.compileShader(strMeshVertexShader.c_str(), GLSLShader::VERTEX);
		MeshShader.compileShader(strMeshFragmentShader.c_str(), GLSLShader::FRAGMENT);
		MeshShader.compileShader(strMeshGeometryShader.c_str(), GLSLShader::GEOMETRY);
		MeshShader.link();
		CubeShader.compileShader(strCubeVertexShader.c_str(), GLSLShader::VERTEX);
		CubeShader.compileShader(strCubeFragmentShader.c_str(), GLSLShader::FRAGMENT);		
		CubeShader.link();	
	}
	catch (ShaderLibException & e) {
		cerr << e.what() << endl;
		system("pause");
	//	exit(EXIT_FAILURE);
	}
}

void initScene(){
	M = glm::mat4(1.0f);
	V = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	P = glm::perspective(45.0f, 1.0f, 0.1f, 100.0f);

	CreateBuffer(MeshShader.getProgram(), vao[0], vbo[0], ibo[0], normal[0]);

	for (int i = 1; i < 3;i++)
		CreateBuffer(CubeShader.getProgram(), vao[i], vbo[i], ibo[i],normal[i]);
	MeshShader.use();
	MeshShader.setUniform("Material.Kd", 0.7f, 0.7f, 0.7f);
	MeshShader.setUniform("Light.Position", vec4(2.0f, 1.0f, 1.0f, 1.0f));
	MeshShader.setUniform("Material.Ka", 0.2f, 0.2f, 0.2f);
	MeshShader.setUniform("Light.Intensity", 0.6f, 0.6f, 0.6f);
	MeshShader.setUniform("Material.Ks", 0.8f, 0.8f, 0.8f);
	MeshShader.setUniform("Material.Shininess", 100.0f);
	
	glClearColor(colBackground.x, colBackground.y, colBackground.z, 1.0f);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
}
void printGlInfo()
{
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}
Drawer::Drawer(int* argc, char** argv){

	/* Initialize the GLUT window */
	glutInit(argc, argv);
	
	int nResX = GetSystemMetrics(SM_CXSCREEN);
	int nResY = GetSystemMetrics(SM_CYSCREEN);	
	nWindowHeight = nResY - 100;
	nBase = nWindowHeight - nPolynomialHeight;	
	nWindowWidth = nBase + nBarWidth;
	glutInitWindowSize(nWindowWidth, nWindowHeight);
	glutInitWindowPosition((nResX - nWindowWidth)/2, 0);
	glutSetOption(GLUT_MULTISAMPLE, 4); //Set number of samples per pixel
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	windowID = glutCreateWindow("Marching Cube");

	glewInit();
	ImGui_ImplGlut_Init();
	CompileAndLinkShader();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
 	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPointSize(8.0);		
//	glCullFace()//剔除面
	/* Callback functions */
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutKeyboardUpFunc(keyboard_up);
	glutSpecialUpFunc(special_up);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(motion);
	glutReshapeFunc(reshape);
	glutIdleFunc(idle);		
	initScene();	
	printGlInfo();
	m_pEvaluator = nullptr;
	m_pMmarching = nullptr;
}

bool Drawer::SetMarch(Marching* m){
	m_pMmarching = m;
	return true;
}

bool Drawer::GetPolyData()
{	
	if (m_pMmarching)
		pData = m_pMmarching->get_poly_data();
	if (pData)
		return true;
	else
		return false;
}

void Drawer::Recalculate()
{
	if (m_pMmarching)	
		m_pMmarching->recalculate();
	if(pData)
		vNormal = CalculateNormal(pData);
}

void Drawer::start(){
	/* Start the main GLUT loop */
	/* NOTE: No code runs after this */	
	pDrawer = this;
	SetScaling(fScaler); 	
	glutMainLoop();
	if (bPlaying || bPause) {
		bPlaying = false;
		bPause = false;
		movie.join();
	}
}

void Drawer::SetGridSize(float fGrid)
{
	if (m_pMmarching)
		m_pMmarching->set_grid_step_size(fGrid);
}

void Drawer::SetStepMode(bool mode)
{
	if (m_pMmarching)
		m_pMmarching->step_by_step_mode(mode);
}

bool Drawer::SetEquation(string s){	
	if (m_pEvaluator)
		return m_pEvaluator->set_equation(s);
}

void Drawer::ResetStep(){
	if (m_pMmarching)
		m_pMmarching->reset_step();
}

void Drawer::LoadFile()
{
	if (m_pMmarching)
		m_pMmarching->load_poly_from_file();
}

void Drawer::SaveFile()
{
	if (m_pMmarching)
		m_pMmarching->save_poly_to_file();
}

void Drawer::SetSeedMode(bool bSeed)
{
	if (m_pMmarching)
		m_pMmarching->seed_mode(bSeed);
}

bool Drawer::SetSeed(float fSeed[3])
{
	if (m_pMmarching)
		return m_pMmarching->set_seed(fSeed[0], fSeed[1], fSeed[2]);
	else
		return false;
}

void Drawer::SetScaling(float fScale[3])
{
	if (m_pMmarching){
		m_pMmarching->set_scaling_x(fScale[0]);
		m_pMmarching->set_scaling_y(fScale[1]);
		m_pMmarching->set_scaling_z(fScale[2]);
	}		
}

bool Drawer::SetConstraint0(string lhs, string op, float rhs)
{
	if (m_pMmarching)
		return m_pMmarching->set_constraint0(lhs,  op,  rhs);
	return false;
}

bool Drawer::SetConstraint1(string lhs, string op, float rhs)
{
	if (m_pMmarching)
		return m_pMmarching->set_constraint1(lhs, op, rhs);
	return false;
}

bool Drawer::SetConstraint2(string lhs, string op, float rhs)
{
	if (m_pMmarching)
		return m_pMmarching->set_constraint2(lhs, op, rhs);
	return false;
}

bool Drawer::UseExtraConstraint0(bool b)
{
	if (m_pMmarching)
		return m_pMmarching->use_constraint0(b);
	return false;
}

bool Drawer::UseExtraConstraint1(bool b)
{
	if (m_pMmarching)
		return m_pMmarching->use_constraint1(b);
	return false;
}

bool Drawer::UseExtraConstraint2(bool b)
{
	if (m_pMmarching)
		return m_pMmarching->use_constraint2(b);
	return false;
}

void Drawer::SetEvaluator(Evaluator* e){
	m_pEvaluator = e;
}

void Drawer::SetSurfaceConstant(float fConstant)
{
	if (m_pMmarching)
		m_pMmarching->set_surface_constant(fConstant);
}

bool Drawer::LoadEquation(string &s)
{
	if (m_pEvaluator)
		return m_pEvaluator->get_equation_from_file(s);
	return false;
}

void Drawer::SaveEquation()
{
	if (m_pEvaluator)
		m_pEvaluator->save_equation_to_file();
}

void Drawer::ResetAlldata()
{
	if (m_pMmarching)
		m_pMmarching->reset_all_data();
}
