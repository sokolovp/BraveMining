/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* global chrome */

const { bindActionCreators } = require('redux')
const newTabActions = require('./actions/newTabActions')
const debounce = require('../common/debounce')

let actions
const getActions = () => {
  if (actions) {
    return actions
  }
  const store = require('./store')
  actions = bindActionCreators(newTabActions, store.dispatch.bind(store))
  return actions
}

/**
 * Obtains a letter / char that represents the current site
 */
const getLetterFromSite = (site) => {
  let name
  try {
    name = new window.URL(site.url || '').hostname
  } catch (e) {}
  name = site.title || name || '?'
  return name.charAt(0).toUpperCase()
}

/**
 * Obtains the top sites and submits an action with the results
 * @param {Array<Object>} - An array of top site objects with title and url.
 */
export const fetchTopSites = (topSites) => {
  chrome.topSites.get(
    (topSites) => getActions().topSitesDataUpdated(topSites || [])
  )
}

/**
 * Obtains the URL's bookmark info and calls an action with the result
 */
export const fetchBookmarkInfo = (url) => {
  chrome.bookmarks.search(url.replace(/^https?:\/\//, ''),
    (bookmarkTreeNodes) => getActions().bookmarkInfoAvailable(url, bookmarkTreeNodes[0])
  )
}

export const getGridSites = (state, checkBookmarkInfo) => {
  const sizeToCount = {large: 18, medium: 12, small: 6}
  const count = sizeToCount[state.gridLayoutSize || 'small']

  // Start with top sites with filtered out ignored sites and pinned sites
  let gridSites = state.topSites.slice()
    .filter((site) =>
      !state.ignoredTopSites.find((ignoredSite) => ignoredSite.url === site.url) &&
      !state.pinnedTopSites.find((pinnedSite) => pinnedSite.url === site.url)
    )

  // Then add in pinned sites at the specified index, these need to be added in the same
  // order as the index they are.
  const pinnedTopSites = state.pinnedTopSites
    .slice()
    .sort((x, y) => x.index - y.index)
  pinnedTopSites.forEach((pinnedSite) => {
    gridSites.splice(pinnedSite.index, 0, pinnedSite)
  })

  gridSites = gridSites.slice(0, count)
  gridSites.forEach((gridSite) => {
    gridSite.letter = getLetterFromSite(gridSite)
    gridSite.thumb = `chrome://thumb/${gridSite.url}`
    gridSite.favicon = `chrome://favicon/size/48@1x/${gridSite.url}`
    gridSite.bookmarked = state.bookmarks[gridSite.url]
    if (checkBookmarkInfo && gridSite.bookmarked === undefined) {
      fetchBookmarkInfo(gridSite.url)
    }
  })
  return gridSites
}

/**
 * Calculates the top sites grid and calls an action with the results
 */
export const calculateGridSites = debounce((state) => {
  getActions().gridSitesUpdated(getGridSites(state, true, true))
}, 10)
