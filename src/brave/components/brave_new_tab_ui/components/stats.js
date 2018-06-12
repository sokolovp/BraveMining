/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const React = require('react')

class Stats extends React.Component {
  get millisecondsPerItem () {
    return 50
  }
  get trackedBlockersCount () {
    return this.props.stats.trackersBlockedStat || 0
  }
  get adblockCount () {
    return this.props.stats.adsBlockedStat || 0
  }
  get httpsUpgradedCount () {
    return this.props.stats.httpsUpgradesStat || 0
  }
  get estimatedTimeSaved () {
    const estimatedMillisecondsSaved = (this.adblockCount + this.trackedBlockersCount) * this.millisecondsPerItem || 0
    const hours = estimatedMillisecondsSaved < 1000 * 60 * 60 * 24
    const minutes = estimatedMillisecondsSaved < 1000 * 60 * 60
    const seconds = estimatedMillisecondsSaved < 1000 * 60
    let counter
    let id

    if (seconds) {
      counter = Math.ceil(estimatedMillisecondsSaved / 1000)
      id = counter === 1 ? 'second' : 'seconds'
    } else if (minutes) {
      counter = Math.ceil(estimatedMillisecondsSaved / 1000 / 60)
      id = counter === 1 ? 'minute' : 'minutes'
    } else if (hours) {
      // Refer to http://stackoverflow.com/a/12830454/2950032 for the detailed reasoning behind the + after
      // toFixed is applied. In a nutshell, + is used to discard unnecessary trailing 0s after calling toFixed
      counter = +((estimatedMillisecondsSaved / 1000 / 60 / 60).toFixed(1))
      id = counter === 1 ? 'hour' : 'hours'
    } else {
      // Otherwise the output is in days
      counter = +((estimatedMillisecondsSaved / 1000 / 60 / 60 / 24).toFixed(2))
      id = counter === 1 ? 'day' : 'days'
    }

    return {
      id,
      value: counter,
      args: JSON.stringify({ value: counter })
    }
  }
  render () {
    const trackedBlockersCount = this.trackedBlockersCount
    const adblockCount = this.adblockCount
    const httpsUpgradedCount = this.httpsUpgradedCount
    const timeSaved = this.estimatedTimeSaved
    const blockedArgs = JSON.stringify({
      adblockCount,
      trackedBlockersCount,
      httpsUpgradedCount
    })
    return <ul className='statsContainer'>
      <li className='statsBlock'>
        <span className='counter trackers'>{trackedBlockersCount.toLocaleString()}</span>
        <span className='statsText' i18n-content='trackersBlocked' data-l10n-args={blockedArgs} />
      </li>
      <li className='statsBlock'>
        <span className='counter ads'>{adblockCount.toLocaleString()}</span>
        <span className='statsText' i18n-content='adsBlocked' data-l10n-args={blockedArgs} />
      </li>
      <li className='statsBlock'>
        <span className='counter https'>{httpsUpgradedCount.toLocaleString()}</span>
        <span className='statsText' i18n-content='httpsUpgraded' data-l10n-args={blockedArgs} />
      </li>
      <li className='statsBlock'>
        <span className='counter timeSaved'>
          {timeSaved.value} <span className='text' i18n-content={timeSaved.id} />
        </span>
        <span className='statsText' i18n-content='estimatedTimeSaved' />
      </li>
    </ul>
  }
}
module.exports = Stats
