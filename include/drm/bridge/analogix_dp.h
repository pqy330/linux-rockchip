#ifndef _ANALOGIX_DP_H_
#define _ANALOGIX_DP_H_

#include <drm/drm_crtc.h>

struct analogix_dp_plat_data {
	struct drm_panel *panel;

	int (*power_on)(struct analogix_dp_plat_data *);
	int (*power_off)(struct analogix_dp_plat_data *);
	int (*attach)(struct analogix_dp_plat_data *, struct drm_bridge *);
	int (*get_modes)(struct analogix_dp_plat_data *,
			 struct drm_connector *);
};

int analogix_dp_resume(struct device *dev);
int analogix_dp_suspend(struct device *dev);

int analogix_dp_bind(struct device *dev, struct drm_device *drm_dev,
		     struct drm_encoder *encoder,
		     struct analogix_dp_plat_data *plat_data);
void analogix_dp_unbind(struct device *dev, struct device *master, void *data);

#endif /* _ANALOGIX_DP_H_ */
