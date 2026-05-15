#pragma once
#include "platform.hpp"
#include "../../common/user_interface_customization.hpp"

/**
 * Thumbnail renderer allows for rendering a preview of what the game is
 * going to look like.
 */
class ThumbnailRenderer
{
      public:
        virtual void
        render_thumbnail(const Platform &platform,
                         const UserInterfaceCustomization &customization) = 0;
};
