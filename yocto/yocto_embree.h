// Author: Cristian Di Pietrantonio
// Implements an interface to Intel Embree from yocto lybrary
#include <embree2\rtcore.h>
#include <embree2\rtcore_ray.h>
#include "yocto_gl.h"
namespace embr {

	// Create an EmbreeScene from a yocto scene
	EmbreeScene scene_from_yocto(ygl::scene *scn);

	// Creates a new triangle mesh from a yocto shape (assuming it is formed only by triangles)
	// and adds it to the scene
	embr::Mesh *triangle_mesh_from_yocto(ygl::shape *triangleShape, EmbreeScene &scene);

	// Creates a new line mesh from a yocto shape
	embr::Mesh *line_mesh_from_yocto(ygl::shape *lineShape, EmbreeScene &scene);

	// Create an instance
	embr::Instance *create_instance(EmbreeScene &scene, Mesh *mesh, ygl::mat4f &xform, unsigned int instID = -1);

	// convert yocto pos to vertex
	inline Vertex vertex_from_pos(ygl::vec3f pos) {
		return { pos.x, pos.y, pos.z, 0 };
	}

	// convert yocto vec31 to triangle
	inline Triangle triangle_from_vec3i(ygl::vec3i tri) {
		return { tri.x, tri.y, tri.z };
	}

	// build a EMbree ray
	inline RTCRay build_ray(float *o, float *d, float tmin, float tmax) {
		RTCRay eRay;
		eRay.org[0] = o[0];
		eRay.org[1] = o[1];
		eRay.org[2] = o[2];
		eRay.dir[0] = d[0];
		eRay.dir[1] = d[1];
		eRay.dir[2] = d[2];
		eRay.tnear = tmin;
		eRay.tfar = tmax;
		eRay.instID = RTC_INVALID_GEOMETRY_ID;
		eRay.geomID = RTC_INVALID_GEOMETRY_ID;
		eRay.primID = RTC_INVALID_GEOMETRY_ID;
		eRay.mask = 0xFFFFFFFF;
		eRay.time = 0.0f;
		return eRay;
	}

	// Intersect a scene
	inline void intersect_scene(const embr::EmbreeScene &escene, RTCRay &eRay) {
		rtcIntersect(escene.mainScene, eRay);
	}

}