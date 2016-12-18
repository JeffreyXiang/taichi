#include <taichi/visual/texture.h>
#include <taichi/visualization/image_buffer.h>

TC_NAMESPACE_BEGIN

class ConstantTexture final : public Texture {
private:
	Vector3 val;
public:
	void initialize(const Config &config) override {
		val = config.get_vec3("value");
	}

	virtual Vector3 sample(const Vector3 &coord) const override {
		return val;
	}
};

TC_IMPLEMENTATION(Texture, ConstantTexture, "const");

class ImageTexture : public Texture {
protected:
	ImageBuffer<Vector3> image;
public:
	void initialize(const Config &config) override {
		image.load(config.get_string("filename"));
	}

	virtual Vector3 sample(const Vector3 &coord_) const override {
		Vector2 coord(coord_.x - floor(coord_.x), coord_.y - floor(coord_.y));
		return image.sample_relative_coord(coord);
	}
};

TC_IMPLEMENTATION(Texture, ImageTexture, "image");

class RectTexture : public Texture {
protected:
	Vector3 bounds;
public:
	void initialize(const Config &config) override {
		bounds = config.get_vec3("bounds") * 0.5f;
	}

	bool inside(const Vector3 &coord) const {
		Vector3 c = coord - Vector3(0.5f);
		return 
			std::abs(c.x) < bounds.x &&
			std::abs(c.y) < bounds.y &&
			std::abs(c.z) < bounds.z;
	}

	virtual Vector3 sample(const Vector3 &coord) const override {
		return Vector3(inside(coord) ? 1.0f : 0.0f);
	}
};

TC_IMPLEMENTATION(Texture, RectTexture, "rect");

class RingTexture : public Texture {
protected:
	real inner, outer;
public:
	void initialize(const Config &config) override {
		inner = config.get("inner", 0.0f) / 2.0f;
		outer = config.get("outer", 1.0f) / 2.0f;
	}

	static bool inside(Vector2 p, Vector2 c, real r) {
		return (p.x - c.x) * (p.x - c.x) + (p.y - c.y) * (p.y - c.y) <= r * r;
	}

	virtual Vector3 sample(const Vector2 &coord) const override {
		return Vector3(
			inside(coord, Vector2(0.5f, 0.5f), outer) &&
			!inside(coord, Vector2(0.5f, 0.5f), inner) ? 1.0f : 0.0f);
	}

	virtual Vector3 sample(const Vector3 &coord) const override {
		return sample(Vector2(coord.x, coord.y));
	}
};

TC_IMPLEMENTATION(Texture, RingTexture, "ring");

class TaichiTexture : public Texture {
protected:
	real scale;
	real rotation_c, rotation_s;
public:
	void initialize(const Config &config) override {
		scale = config.get("scale", 1.0f);
		real rotation = config.get("rotation", 0.0f);
		rotation_c = std::cos(rotation);
		rotation_s = std::sin(rotation);
	}

	static bool inside(Vector2 p, Vector2 c, real r) {
		return (p.x - c.x) * (p.x - c.x) + (p.y - c.y) * (p.y - c.y) <= r * r;
	}

	static bool inside_left(Vector2 p, Vector2 c, real r) {
		return inside(p, c, r) && p.x < c.x;
	}

	static bool inside_right(Vector2 p, Vector2 c, real r) {
		return inside(p, c, r) && p.x >= c.x;
	}

	bool is_white(Vector2 p_) const {
		p_ -= Vector2(0.5f);
		Vector2 p(p_.x * rotation_c + p_.y * rotation_s, -p_.x * rotation_s + p_.y * rotation_c);
		p += Vector2(0.5f);
		if (!inside(p, Vector2(0.50f, 0.50f), 0.5f)) {
			return true;
		}
		if (!inside(p, Vector2(0.50f, 0.50f), 0.5f * scale)) {
			return false;
		}
		p = Vector2(0.5f) + (p - Vector2(0.5f)) * (1.0f / scale);
		if (inside(p, Vector2(0.50f, 0.25f), 0.08f)) {
			return true;
		}
		if (inside(p, Vector2(0.50f, 0.75f), 0.08f)) {
			return false;
		}
		if (inside(p, Vector2(0.50f, 0.25f), 0.25f)) {
			return false;
		}
		if (inside(p, Vector2(0.50f, 0.75f), 0.25f)) {
			return true;
		}
		if (p.x < 0.5f) {
			return true;
		}
		else {
			return false;
		}
	}

	virtual Vector3 sample(const Vector2 &coord) const override {
		return Vector3(is_white(coord) ? 1.0f : 0.0f);
	}

	virtual Vector3 sample(const Vector3 &coord) const override {
		return sample(Vector2(coord.x, coord.y));
	}
};

TC_IMPLEMENTATION(Texture, TaichiTexture, "taichi");

class CheckerboardTexture : public Texture {
protected:
	real repeat_u, repeat_v, repeat_w;
	std::shared_ptr<Texture> tex1, tex2;
public:
	void initialize(const Config &config) override {
		tex1 = AssetManager::get_asset<Texture>(config.get_int("tex1"));
		tex2 = AssetManager::get_asset<Texture>(config.get_int("tex2"));
		repeat_u = config.get_real("repeat_u");
		repeat_v = config.get_real("repeat_v");
		repeat_w = config.get("repeat_w", 1.0f);
	}

	virtual Vector3 sample(const Vector3 &coord) const override {
		int p = (int)floor(coord.x * repeat_u), q = (int)floor(coord.y * repeat_v),
			r = (int)floor(coord.z * repeat_w);
		return ((p + q + r) % 2 == 0 ? tex1 : tex2)->sample(coord);
	}
};

TC_IMPLEMENTATION(Texture, CheckerboardTexture, "checkerboard");

TC_NAMESPACE_END

