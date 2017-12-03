#include <json-c/json.h>
#include <stdio.h>
#include <ctype.h>
#include "log.h"
#include "sway/ipc-json.h"
#include "sway/container.h"
#include <wlr/types/wlr_box.h>
#include <wlr/types/wlr_output.h>

json_object *ipc_json_get_version() {
	int major = 0, minor = 0, patch = 0;
	json_object *version = json_object_new_object();

	sscanf(SWAY_VERSION, "%u.%u.%u", &major, &minor, &patch);

	json_object_object_add(version, "human_readable", json_object_new_string(SWAY_VERSION));
	json_object_object_add(version, "variant", json_object_new_string("sway"));
	json_object_object_add(version, "major", json_object_new_int(major));
	json_object_object_add(version, "minor", json_object_new_int(minor));
	json_object_object_add(version, "patch", json_object_new_int(patch));

	return version;
}

static json_object *ipc_json_create_rect(swayc_t *c) {
	json_object *rect = json_object_new_object();

	json_object_object_add(rect, "x", json_object_new_int((int32_t)c->x));
	json_object_object_add(rect, "y", json_object_new_int((int32_t)c->y));
	json_object_object_add(rect, "width", json_object_new_int((int32_t)c->width));
	json_object_object_add(rect, "height", json_object_new_int((int32_t)c->height));

	return rect;
}

static void ipc_json_describe_root(swayc_t *root, json_object *object) {
	json_object_object_add(object, "type", json_object_new_string("root"));
	json_object_object_add(object, "layout", json_object_new_string("splith"));
}

static void ipc_json_describe_output(swayc_t *output, json_object *object) {
	json_object_object_add(object, "type", json_object_new_string("output"));
	json_object_object_add(object, "current_workspace",
		(output->focused) ? json_object_new_string(output->focused->name) : NULL);
}

static void ipc_json_describe_workspace(swayc_t *workspace, json_object *object) {
	int num = (isdigit(workspace->name[0])) ? atoi(workspace->name) : -1;

	json_object_object_add(object, "num", json_object_new_int(num));
	json_object_object_add(object, "output", (workspace->parent) ? json_object_new_string(workspace->parent->name) : NULL);
	json_object_object_add(object, "type", json_object_new_string("workspace"));
}

static void ipc_json_describe_view(swayc_t *c, json_object *object) {
	json_object_object_add(object, "name", (c->name) ? json_object_new_string(c->name) : NULL);
}

json_object *ipc_json_describe_container(swayc_t *c) {
	if (!(sway_assert(c, "Container must not be null."))) {
		return NULL;
	}

	json_object *object = json_object_new_object();

	json_object_object_add(object, "id", json_object_new_int((int)c->id));
	json_object_object_add(object, "name", (c->name) ? json_object_new_string(c->name) : NULL);
	json_object_object_add(object, "rect", ipc_json_create_rect(c));

	switch (c->type) {
	case C_ROOT:
		ipc_json_describe_root(c, object);
		break;

	case C_OUTPUT:
		ipc_json_describe_output(c, object);
		break;

	case C_CONTAINER:
	case C_VIEW:
		ipc_json_describe_view(c, object);
		break;

	case C_WORKSPACE:
		ipc_json_describe_workspace(c, object);
		break;

	case C_TYPES:
	default:
		break;
	}

	return object;
}

json_object *ipc_json_describe_container_recursive(swayc_t *c) {
	json_object *object = ipc_json_describe_container(c);
	int i;

	json_object *children = json_object_new_array();
	if (c->type != C_VIEW && c->children) {
		for (i = 0; i < c->children->length; ++i) {
			json_object_array_add(children, ipc_json_describe_container_recursive(c->children->items[i]));
		}
	}
	json_object_object_add(object, "nodes", children);

	return object;
}
