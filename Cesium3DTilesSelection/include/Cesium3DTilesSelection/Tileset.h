#pragma once

#include <Cesium3DTilesSelection/Library.h>
#include <Cesium3DTilesSelection/LoadedTileEnumerator.h>
#include <Cesium3DTilesSelection/RasterOverlayCollection.h>
#include <Cesium3DTilesSelection/SampleHeightResult.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderFactory.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetLoadFailureDetails.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <Cesium3DTilesSelection/TilesetViewGroup.h>
#include <Cesium3DTilesSelection/ViewState.h>
#include <Cesium3DTilesSelection/ViewUpdateResult.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <rapidjson/fwd.h>

#include <list>
#include <memory>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

class TilesetContentManager;
class TilesetMetadata;
class TilesetHeightQuery;
struct TilesetHeightRequest;
class TilesetSharedAssetSystem;
class TilesetViewGroup;
class TilesetFrameState;

/**
 * @brief A <a
 * href="https://github.com/CesiumGS/3d-tiles/tree/master/specification">3D
 * Tiles tileset</a>, used for streaming massive heterogeneous 3D geospatial
 * datasets.
 */
class CESIUM3DTILESSELECTION_API Tileset final {
public:
  /**
   * @brief Constructs a new instance with a given custom tileset loader.
   * @param externals The external interfaces to use.
   * @param pCustomLoader The custom loader used to load the tileset and tile
   * content.
   * @param pRootTile The root tile that is associated with the custom loader
   * @param options Additional options for the tileset.
   */
  Tileset(
      const TilesetExternals& externals,
      std::unique_ptr<TilesetContentLoader>&& pCustomLoader,
      std::unique_ptr<Tile>&& pRootTile,
      const TilesetOptions& options = TilesetOptions());

  /**
   * @brief Constructs a new instance with a given `tileset.json` URL.
   * @param externals The external interfaces to use.
   * @param url The URL of the `tileset.json`.
   * @param options Additional options for the tileset.
   */
  Tileset(
      const TilesetExternals& externals,
      const std::string& url,
      const TilesetOptions& options = TilesetOptions());

  /**
   * @brief Constructs a new instance with the given asset ID on <a
   * href="https://cesium.com/ion/">Cesium ion</a>.
   * @param externals The external interfaces to use.
   * @param ionAssetID The ID of the Cesium ion asset to use.
   * @param ionAccessToken The Cesium ion access token authorizing access to the
   * asset.
   * @param options Additional options for the tileset.
   * @param ionAssetEndpointUrl The URL of the ion asset endpoint. Defaults
   * to Cesium ion but a custom endpoint can be specified.
   */
  Tileset(
      const TilesetExternals& externals,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      const TilesetOptions& options = TilesetOptions(),
      const std::string& ionAssetEndpointUrl = "https://api.cesium.com/");

  /**
   * @brief Constructs a new instance with the given @ref
   * TilesetContentLoaderFactory.
   *
   * @param externals The external interfaces to use.
   * @param loaderFactory The factory to use to create the @ref
   * TilesetContentLoader.
   * @param options Additional options for the tileset.
   */
  Tileset(
      const TilesetExternals& externals,
      TilesetContentLoaderFactory&& loaderFactory,
      const TilesetOptions& options = TilesetOptions());

  /**
   * @brief Destroys this tileset.
   *
   * Destroying the tileset will immediately (before the destructor returns)
   * unload as much tile content as possible. However, tiles that are currently
   * in the process of being loaded cannot be unloaded immediately. These tiles
   * will be unloaded asynchronously some time after this destructor returns. To
   * be notified of completion of the async portion of the tileset destruction,
   * subscribe to {@link getAsyncDestructionCompleteEvent}.
   */
  ~Tileset() noexcept;

  /**
   * @brief A future that resolves when this Tileset has been destroyed (i.e.
   * its destructor has been called) and all async operations that it was
   * executing have completed.
   */
  CesiumAsync::SharedFuture<void>& getAsyncDestructionCompleteEvent();

  /**
   * @brief A future that resolves when the details of the root tile of this
   * tileset are available. The root tile's content (e.g., 3D model), however,
   * will not necessarily be loaded yet.
   */
  CesiumAsync::SharedFuture<void>& getRootTileAvailableEvent();

  /**
   * @brief Gets the {@link CesiumUtility::Credit} created from
   * {@link TilesetOptions::credit}, if any.
   */
  std::optional<CesiumUtility::Credit> getUserCredit() const noexcept;

  /**
   * @brief Get tileset credits.
   */
  const std::vector<CesiumUtility::Credit>& getTilesetCredits() const noexcept;

  /**
   * @brief Sets whether or not the tileset's credits should be shown on screen.
   * @param showCreditsOnScreen Whether the credits should be shown on screen.
   */
  void setShowCreditsOnScreen(bool showCreditsOnScreen) noexcept;

  /**
   * @brief Gets the {@link TilesetExternals} that summarize the external
   * interfaces used by this tileset.
   */
  TilesetExternals& getExternals() noexcept { return this->_externals; }

  /**
   * @brief Gets the {@link TilesetExternals} that summarize the external
   * interfaces used by this tileset.
   */
  const TilesetExternals& getExternals() const noexcept {
    return this->_externals;
  }

  /**
   * @brief Returns the {@link CesiumAsync::AsyncSystem} that is used for
   * dispatching asynchronous tasks.
   */
  CesiumAsync::AsyncSystem& getAsyncSystem() noexcept {
    return this->_asyncSystem;
  }

  /** @copydoc Tileset::getAsyncSystem() */
  const CesiumAsync::AsyncSystem& getAsyncSystem() const noexcept {
    return this->_asyncSystem;
  }

  /** @copydoc Tileset::getOptions() */
  const TilesetOptions& getOptions() const noexcept { return this->_options; }

  /**
   * @brief Gets the {@link TilesetOptions} of this tileset.
   */
  TilesetOptions& getOptions() noexcept { return this->_options; }

  /**
   * @brief Gets the {@link CesiumGeospatial::Ellipsoid} used by this tileset.
   */
  const CesiumGeospatial::Ellipsoid& getEllipsoid() const {
    return this->_options.ellipsoid;
  }

  /** @copydoc Tileset::getEllipsoid */
  CesiumGeospatial::Ellipsoid& getEllipsoid() noexcept {
    return this->_options.ellipsoid;
  }

  /**
   * @brief Gets the root tile of this tileset.
   *
   * This may be `nullptr` if there is currently no root tile.
   */
  Tile* getRootTile() noexcept;

  /** @copydoc Tileset::getRootTile() */
  const Tile* getRootTile() const noexcept;

  /**
   * @brief Returns the {@link RasterOverlayCollection} of this tileset.
   */
  RasterOverlayCollection& getOverlays() noexcept;

  /** @copydoc Tileset::getOverlays() */
  const RasterOverlayCollection& getOverlays() const noexcept;

  /**
   * @brief Returns the {@link TilesetSharedAssetSystem} of this tileset.
   */
  TilesetSharedAssetSystem& getSharedAssetSystem() noexcept;

  /** @copydoc Tileset::getSharedAssetSystem() */
  const TilesetSharedAssetSystem& getSharedAssetSystem() const noexcept;

  /**
   * @brief Updates this view but waits for all tiles that meet sse to finish
   * loading and ready to be rendered before returning the function. This method
   * is significantly slower than {@link Tileset::updateView} and should only be
   * used for capturing movie or non-realtime situation.
   * @param frustums The {@link ViewState}s that the view should be updated for
   * @returns The set of tiles to render in the updated view. This value is only
   * valid until the next call to `updateView` or until the tileset is
   * destroyed, whichever comes first.
   */
  [[deprecated("Instead of `tileset.updateViewOffline(...)`, call "
               "`tileset.updateViewGroupOffline(tileset.getDefaultViewGroup(), "
               "...)`.")]] const ViewUpdateResult&
  updateViewOffline(const std::vector<ViewState>& frustums);

  /**
   * @brief Updates this view, returning the set of tiles to render in this
   * view.
   *
   * Calling this method is equivalent to calling {@link updateViewGroup} with
   * the default view group ({@link getDefaultViewGroup}) and then calling
   * {@link loadTiles}.
   *
   * @param frustums The {@link ViewState}s that the view should be updated for
   * @param deltaTime The amount of time that has passed since the last call to
   * updateView, in seconds.
   * @returns The set of tiles to render in the updated view. This value is only
   * valid until the next call to `updateView` or until the tileset is
   * destroyed, whichever comes first.
   */
  [[deprecated("Instead of `tileset.updateView(...)`, call "
               "`tileset.updateViewGroup(tileset.getDefaultViewGroup(), ...)` "
               "followed by `tileset.loadTiles()`.")]] const ViewUpdateResult&
  updateView(const std::vector<ViewState>& frustums, float deltaTime = 0.0f);

  /**
   * @brief Gets the total number of tiles that are currently loaded.
   */
  int32_t getNumberOfTilesLoaded() const;

  /**
   * @brief Gets the percentage of tiles that had been loaded for the default
   * view group as of the last time it was updated.
   *
   * To get the load progress of a view group other than the default, call
   * {@link TilesetViewGroup::getPreviousLoadProgressPercentage}.
   */
  float computeLoadProgress() noexcept;

  /**
   * @brief Gets an object that can be used to enumerate the loaded tiles of
   * this tileset.
   *
   * Before the root tile is available, this method will return an enumerator
   * that is empty, and that instance will remain empty even after the root tile
   * is available.

   * Once the root tile is available (see {@link getRootTile} and
   * {@link getRootTileAvailableEvent}), the returned instance will remain
   * valid until the tileset is destroyed.
   *
   * While the returned enumerator itself will remain valid as long as the
   * `Tileset` does, a given iteration may be invalidated by any operation that
   * modifies the {@link Tile} hierarchy.
   */
  LoadedConstTileEnumerator loadedTiles() const;

  /** @copydoc loadedTiles */
  LoadedTileEnumerator loadedTiles();

  /**
   * @brief Invokes a function for each tile that is currently loaded.
   *
   * @param callback The function to invoke.
   */
  void forEachLoadedTile(const std::function<void(Tile& tile)>& callback);

  /**
   * @brief Invokes a function for each tile that is currently loaded.
   *
   * @param callback The function to invoke.
   */
  void forEachLoadedTile(
      const std::function<void(const Tile& tile)>& callback) const;

  /**
   * @brief Gets the total number of bytes of tile and raster overlay data that
   * are currently loaded.
   */
  int64_t getTotalDataBytes() const noexcept;

  /**
   * @brief Gets the {@link TilesetMetadata} associated with the main or
   * external tileset.json that contains a given tile. If the metadata is not
   * yet loaded, this method returns nullptr.
   *
   * If this tileset's root tile is not yet available, this method returns
   * nullptr.
   *
   * If the tileset has a {@link TilesetMetadata::schemaUri}, it will not
   * necessarily have been loaded yet.
   *
   * If the provided tile is not the root tile of a tileset.json, this method
   * walks up the {@link Tile::getParent} chain until it finds the closest
   * root and then returns the metadata associated with the corresponding
   * tileset.json.
   *
   * Consider calling {@link loadMetadata} instead, which will return a future
   * that only resolves after the root tile is loaded and the `schemaUri`, if
   * any, has been resolved.
   *
   * @param pTile The tile. If this parameter is nullptr, the metadata for the
   * main tileset.json is returned.
   * @return The found metadata, or nullptr if the root tile is not yet loaded.
   */
  const TilesetMetadata* getMetadata(const Tile* pTile = nullptr) const;

  /**
   * @brief Asynchronously loads the metadata associated with the main
   * tileset.json.
   *
   * Before the returned future resolves, the root tile of this tileset will be
   * loaded and the {@link TilesetMetadata::schemaUri} will be loaded if one
   * has been specified.
   *
   * If the tileset or `schemaUri` fail to load, the returned future will
   * reject.
   *
   * @return A shared future that resolves to the loaded metadata. Once this
   * future resolves, {@link getMetadata} can be used to synchronously obtain
   * the same metadata instance.
   */
  CesiumAsync::Future<const TilesetMetadata*> loadMetadata();

  /**
   * @brief Initiates an asynchronous query for the height of this tileset at a
   * list of cartographic positions (longitude and latitude). The most detailed
   * available tiles are used to determine each height.
   *
   * The height of the input positions is ignored. The output height is
   * expressed in meters above the ellipsoid (usually WGS84), which should not
   * be confused with a height above mean sea level.
   *
   * Note that {@link Tileset::updateView} must be called periodically, or else
   * the returned `Future` will never resolve. If you are not using this tileset
   * for visualization, you can call `updateView` with an empty list of
   * frustums.
   *
   * @param positions The positions for which to sample heights.
   * @return A future that asynchronously resolves to the result of the height
   * query.
   */
  CesiumAsync::Future<SampleHeightResult> sampleHeightMostDetailed(
      const std::vector<CesiumGeospatial::Cartographic>& positions);

  /**
   * @brief Gets the default view group that is used when calling
   * {@link updateView}.
   *
   * @return The view group.
   */
  TilesetViewGroup& getDefaultViewGroup();

  /** @copydoc getDefaultViewGroup */
  const TilesetViewGroup& getDefaultViewGroup() const;

  /**
   * @brief Updates a view group, returning the set of tiles to render in this
   * view.
   *
   * This method should typically be called once per "render frame", but it may
   * be called at different rates for different view groups.
   *
   * Users must also periodically call {@link loadTiles}, which will start or
   * continue the asynchronous process of loading tiles that are needed across
   * all view groups.
   *
   * @param viewGroup The view group to update. The first time `updateViewGroup`
   * is called, simply create a new `TilesetViewGroup` to pass as this
   * parameter. For successive calls to `updateViewGroup`, pass this same
   * instance.
   * @param frustums The {@link ViewState} instances that are observing the
   * tileset in this view group.
   * @param deltaTime The amount of time that has passed since the last call to
   * updateView, in seconds.
   * @returns The set of tiles to render in the updated view. This value is only
   * valid until the next call to `updateViewGroup` or until the view group is
   * destroyed, whichever comes first.
   */
  const ViewUpdateResult& updateViewGroup(
      TilesetViewGroup& viewGroup,
      const std::vector<ViewState>& frustums,
      float deltaTime = 0.0f);

  /**
   * @brief Updates a view group, returning the set of tiles to render in this
   * view. Unlike {@link updateViewGroup}, this method blocks the calling thread
   * until all tiles suitable for the views have been loaded.
   *
   * This method is significantly slower than {@link updateViewGroup} and
   * should only be used for capturing a movie or for other non-realtime
   * situations.
   *
   * @param viewGroup The view group to update. The first time `updateViewGroup`
   * is called, simply create a new `TilesetViewGroup` to pass as this
   * parameter. For successive calls to `updateViewGroup`, pass this same
   * instance.
   * @param frustums The {@link ViewState} instances that are observing the
   * tileset in this view group.
   * @returns The set of tiles to render in the updated view. This value is only
   * valid until the next call to `updateViewGroup` or until the view group is
   * destroyed, whichever comes first.
   */
  const ViewUpdateResult& updateViewGroupOffline(
      TilesetViewGroup& viewGroup,
      const std::vector<ViewState>& frustums);

  /**
   * @brief Loads the tiles that are currently deemed the most important,
   * across all height queries and {@link TilesetViewGroup} instances.
   *
   * In order to minimize tile load latency, this method should be called
   * frequently, such as once per render frame. It will return quickly when
   * there is no work to do.
   *
   * This method also calls
   * {@link CesiumAsync::AsyncSystem::dispatchMainThreadTasks} on the tileset's
   * {@link getAsyncSystem}.
   */
  void loadTiles();

  /**
   * @brief Registers a tile load requester with this Tileset. Registered tile
   * load requesters get to influence which tiles are loaded when
   * {@link loadTiles} is called.
   *
   * If the given requester is already registered with this Tileset, this method
   * does nothing.
   *
   * To unregister a load requester, call {@link TileLoadRequester::unregister}.
   *
   * @param requester The requester to register.
   */
  void registerLoadRequester(TileLoadRequester& requester);

  Tileset(const Tileset& rhs) = delete;
  Tileset& operator=(const Tileset& rhs) = delete;

private:
  /**
   * @brief The result of traversing one branch of the tile hierarchy.
   *
   * Instances of this structure are created by the `_visit...` functions,
   * and summarize the information that was gathered during the traversal
   * of the respective branch, so that this information can be used by
   * the parent to decide on the further traversal process.
   */
  struct TraversalDetails {
    /**
     * @brief Whether all selected tiles in this tile's subtree are renderable.
     *
     * This is `true` if all selected (i.e. not culled or refined) tiles in this
     * tile's subtree are renderable. If the subtree is renderable, we'll render
     * it; no drama.
     */
    bool allAreRenderable = true;

    /**
     * @brief Whether any tile in this tile's subtree was rendered in the last
     * frame.
     *
     * This is `true` if any tiles in this tile's subtree were rendered last
     * frame. If any were, we must render the subtree rather than this tile,
     * because rendering this tile would cause detail to vanish that was visible
     * last frame, and that's no good.
     */
    bool anyWereRenderedLastFrame = false;

    /**
     * @brief The number of selected tiles in this tile's subtree that are not
     * yet renderable.
     *
     * Counts the number of selected tiles in this tile's subtree that are
     * not yet ready to be rendered because they need more loading. Note that
     * this value will _not_ necessarily be zero when
     * `allAreRenderable` is `true`, for subtle reasons.
     * When `allAreRenderable` and `anyWereRenderedLastFrame` are both `false`,
     * we will render this tile instead of any tiles in its subtree and the
     * `allAreRenderable` value for this tile will reflect only whether _this_
     * tile is renderable. The `notYetRenderableCount` value, however, will
     * still reflect the total number of tiles that we are waiting on, including
     * the ones that we're not rendering. `notYetRenderableCount` is only reset
     * when a subtree is removed from the render queue because the
     * `notYetRenderableCount` exceeds the
     * {@link TilesetOptions::loadingDescendantLimit}.
     */
    uint32_t notYetRenderableCount = 0;
  };

  TraversalDetails _renderLeaf(
      const TilesetFrameState& frameState,
      Tile& tile,
      double tilePriority,
      ViewUpdateResult& result);
  TraversalDetails _renderInnerTile(
      const TilesetFrameState& frameState,
      Tile& tile,
      ViewUpdateResult& result);
  bool _kickDescendantsAndRenderTile(
      const TilesetFrameState& frameState,
      Tile& tile,
      ViewUpdateResult& result,
      TraversalDetails& traversalDetails,
      size_t firstRenderedDescendantIndex,
      const TilesetViewGroup::LoadQueueCheckpoint& loadQueueBeforeChildren,
      bool queuedForLoad,
      double tilePriority);
  TileOcclusionState _checkOcclusion(const Tile& tile);

  TraversalDetails _visitTile(
      const TilesetFrameState& frameState,
      uint32_t depth,
      bool meetsSse,
      bool ancestorMeetsSse,
      Tile& tile,
      double tilePriority,
      ViewUpdateResult& result);

  struct CullResult {
    // whether we should visit this tile
    bool shouldVisit = true;
    // whether this tile was culled (Note: we might still want to visit it)
    bool culled = false;
  };

  // TODO: abstract these into a composable culling interface.
  void _frustumCull(
      const Tile& tile,
      const TilesetFrameState& frameState,
      bool cullWithChildrenBounds,
      CullResult& cullResult);
  void _fogCull(
      const TilesetFrameState& frameState,
      const std::vector<double>& distances,
      CullResult& cullResult);
  bool _meetsSse(
      const std::vector<ViewState>& frustums,
      const Tile& tile,
      const std::vector<double>& distances,
      bool culled) const noexcept;

  TraversalDetails _visitTileIfNeeded(
      const TilesetFrameState& frameState,
      uint32_t depth,
      bool ancestorMeetsSse,
      Tile& tile,
      ViewUpdateResult& result);
  TraversalDetails _visitVisibleChildrenNearToFar(
      const TilesetFrameState& frameState,
      uint32_t depth,
      bool ancestorMeetsSse,
      Tile& tile,
      ViewUpdateResult& result);

  /**
   * @brief When called on an additive-refined tile, queues it for load and adds
   * it to the render list.
   *
   * For replacement-refined tiles, this method does nothing and returns false.
   *
   * @param tile The tile to potentially load and render.
   * @param result The current view update result.
   * @param tilePriority The load priority of this tile.
   * priority.
   * @param queuedForLoad True if this tile has already been queued for loading.
   * @return true The additive-refined tile was queued for load and added to the
   * render list.
   * @return false The non-additive-refined tile was ignored.
   */
  bool _loadAndRenderAdditiveRefinedTile(
      const TilesetFrameState& frameState,
      Tile& tile,
      ViewUpdateResult& result,
      double tilePriority,
      bool queuedForLoad);

  void _unloadCachedTiles(double timeBudget) noexcept;

  void _updateLodTransitions(
      const TilesetFrameState& frameState,
      float deltaTime,
      ViewUpdateResult& result) const noexcept;

  TilesetExternals _externals;
  CesiumAsync::AsyncSystem _asyncSystem;

  TilesetOptions _options;

  // Holds computed distances, to avoid allocating them on the heap during tile
  // selection.
  std::vector<double> _distances;

  // Holds the occlusion proxies of the children of a tile. Store them in this
  // scratch variable so that it can allocate only when growing bigger.
  std::vector<const TileOcclusionRendererProxy*> _childOcclusionProxies;

  CesiumUtility::IntrusivePointer<TilesetContentManager>
      _pTilesetContentManager;

  std::list<TilesetHeightRequest> _heightRequests;

  TilesetViewGroup _defaultViewGroup;

  void addTileToLoadQueue(
      const TilesetFrameState& frameState,
      Tile& tile,
      TileLoadPriorityGroup priorityGroup,
      double priority);

  static TraversalDetails createTraversalDetailsForSingleTile(
      const TilesetFrameState& frameState,
      const Tile& tile);
};

} // namespace Cesium3DTilesSelection
