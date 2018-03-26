#include "yocto_embree.h"
#include <iostream>

using namespace embr;

// error callback
void error_handling(void *userPtr, const RTCError err, const char*string) {
	std::cerr << string<< std::endl;
	return;
}

// create an EmbreeScene from a yocto Scene
embr::EmbreeScene embr::scene_from_yocto(ygl::scene *scn) {
	RTCDevice device = rtcNewDevice();
	if (device == nullptr) {
		std::cerr << "Device is null!" << std::endl;
		return {};
	}
	// set error callback
	rtcDeviceSetErrorFunction2(device, error_handling, NULL);

	// Create primary scene
	RTCScene mainScene = rtcDeviceNewScene(device, RTC_SCENE_STATIC, RTC_INTERSECT1);

	// Create meshes and instances
	std::vector<embr::Mesh*>  *meshes = new std::vector<embr::Mesh*> {};
	std::map<unsigned int, embr::Instance*> *instances = new std::map<unsigned int, embr::Instance*> {};
	
	// Create the embree scene
	EmbreeScene embMainScene = {
		mainScene, device, instances, meshes
	};

	std::map<ygl::shape_group*, int> mesh_mapping;
	
	unsigned int iid = 0;

	for (ygl::instance *inst : scn->instances) {
		// get the instance mesh
		auto mesh = inst->shp;
		// find if that mesh has already been created
		auto fmesh = mesh_mapping.find(mesh);
		unsigned int mid;
		if (fmesh != mesh_mapping.end()) {
			// it is already existing
			mid = fmesh->second;
		}
		else {
			// assume each mesh has only one shape
			auto shp = mesh->shapes[0];
			mid = meshes->size();
			if (shp->triangles.size() > 0) {
				//triangle shape!
				Mesh *tri = triangle_mesh_from_yocto(shp, embMainScene);
				mesh_mapping.insert(std::make_pair(mesh, mid));
				meshes->push_back(tri);
			}
			else if (shp->lines.size() > 0) {
				Mesh *li = line_mesh_from_yocto(shp, embMainScene);
				mesh_mapping.insert(std::make_pair(mesh, mid));
				meshes->push_back(li);
			}
			else{
				iid++;
			continue;
			}
		}
		//add the instance
		instances->insert(std::make_pair(iid, create_instance(embMainScene, meshes->at(mid), ygl::frame_to_mat(inst->frame), iid++)));
	}
	rtcCommit(mainScene);
	return embMainScene;
}

// Creates a new triangle mesh from a yocto shape (assuming it is formed only by triangles)
// and adds it to the scene
embr::Mesh *embr::triangle_mesh_from_yocto(ygl::shape *triangleShape, EmbreeScene &scene) {
	// Create a new scene for the mesh (it is like a frame, meh)
	RTCScene _triangleMesh = rtcDeviceNewScene(scene._device, RTC_SCENE_STATIC, RTC_INTERSECT1);
	unsigned int geomID = rtcNewTriangleMesh2(_triangleMesh, RTC_GEOMETRY_STATIC, triangleShape->triangles.size(), triangleShape->pos.size(), 1, 0);
	// Set the vertices
	Vertex *vertices = (Vertex*) rtcMapBuffer(_triangleMesh, geomID, RTC_VERTEX_BUFFER);
	for (int i = 0; i < triangleShape->pos.size(); i++) {
		vertices[i] = vertex_from_pos(triangleShape->pos[i]);
	}
	rtcUnmapBuffer(_triangleMesh, geomID, RTC_VERTEX_BUFFER);

	Triangle * triangles = (Triangle*)rtcMapBuffer(_triangleMesh, geomID, RTC_INDEX_BUFFER);
	for (int i = 0; i < triangleShape->triangles.size(); i++) {
		triangles[i] = triangle_from_vec3i(triangleShape->triangles[i]);
	}
	rtcUnmapBuffer(_triangleMesh, geomID, RTC_INDEX_BUFFER);
	rtcCommit(_triangleMesh);
	return new embr::Mesh { _triangleMesh};
}

// Creates a new line mesh from a yocto shape (assuming it is formed only by triangles)
// and adds it to the scene
embr::Mesh *embr::line_mesh_from_yocto(ygl::shape *lineShape, EmbreeScene &scene) {
	// Create a new scene for the mesh (it is like a frame, meh)
	RTCScene _lineMesh = rtcDeviceNewScene(scene._device, RTC_SCENE_STATIC, RTC_INTERSECT1);
	unsigned int geomID = 0;
	geomID = rtcNewLineSegments2(_lineMesh, RTC_GEOMETRY_STATIC,lineShape->lines.size(), lineShape->pos.size(), 1, geomID);
	// Set the vertices
	Vertex *vertices = (Vertex*)rtcMapBuffer(_lineMesh, geomID, RTC_VERTEX_BUFFER);
	for (int i = 0; i < lineShape->pos.size(); i++)
		vertices[i] = { lineShape->pos[i].x, lineShape->pos[i].y, lineShape->pos[i].z, lineShape->radius[i] };
	rtcUnmapBuffer(_lineMesh, geomID, RTC_VERTEX_BUFFER);
	int * curves = (int*)rtcMapBuffer(_lineMesh, geomID, RTC_INDEX_BUFFER);
	for (int i = 0; i < lineShape->lines.size(); i++) {
		curves[i] = lineShape->lines[i].x;
	}
	rtcUnmapBuffer(_lineMesh, geomID, RTC_INDEX_BUFFER);
	rtcCommit(_lineMesh);
	return new embr::Mesh{ _lineMesh };
}

embr::Instance *embr::create_instance(EmbreeScene &scene, Mesh *mesh, ygl::mat4f &xform, unsigned int instID) {
	// add an instance of "mesh" into "scene"
	instID = rtcNewInstance3(scene.mainScene, mesh->scene, 1, instID);
	// apply a transformation
	rtcSetTransform2(scene.mainScene, instID, RTC_MATRIX_COLUMN_MAJOR_ALIGNED16, &(xform.x.x));
	return new embr::Instance { instID , mesh};
}
