'use strict'

const path = require('path')
const fs = require('fs')

const packages = require('../package')

const getNPMConfig = (path) => {
  const key = path.join('_').replace('-', '_')
  const npm_prefix = 'npm_config_'
  const package_prefix = 'npm_package_config_'
  return process.env[npm_prefix + key] || process.env[package_prefix + key]
}

const Config = function () {
  this.defaultBuildConfig = 'Release'
  this.buildConfig = this.defaultBuildConfig
  this.projectNames = []
  this.projects = {}
  this.rootDir = path.join(path.dirname(__filename), '..')
  this.scriptDir = path.join(this.rootDir, 'scripts')
  this.depotToolsDir = path.join(this.rootDir, 'vendor', 'depot_tools')
  this.srcDir = path.join(this.rootDir, getNPMConfig(['projects', 'chrome', 'dir']))
  this.buildToolsDir = path.join(this.srcDir, 'build')
  this.resourcesDir = path.join(this.rootDir, 'resources')
  this.defaultGClientFile = path.join(this.rootDir, '.gclient')
  this.gClientFile = process.env.BRAVE_GCLIENT_FILE || this.defaultGClientFile
  this.targetArch = 'x64'
  this.gypTargetArch = 'x64'
  this.officialBuild = true
  this.debugBuild = JSON.parse(getNPMConfig(['brave_debug_build']) || false)
  this.googleApiKey = getNPMConfig(['brave_google_api_key']) || 'AIzaSyAQfxPJiounkhOjODEO5ZieffeBv6yft2Q'
  this.googleApiEndpoint = getNPMConfig(['brave_google_api_endpoint']) || 'https://www.googleapis.com/geolocation/v1/geolocate?key='
  this.buildProjects()
  const braveCoreDirPackage = path.join(this.projects['brave-core'].dir, 'package')
  this.braveCoreVersion = getNPMConfig(['brave_version']) || (fs.existsSync(braveCoreDirPackage + '.json') && require(braveCoreDirPackage)['version']) || ''
  this.releaseTag = this.braveCoreVersion.split('+')[0]
  this.channel = ''
}

Config.prototype.buildArgs = function () {
  const version = this.braveCoreVersion
  let version_parts = version.split('+')[0]
  version_parts = version_parts.split('.')

  let args = {
    root_extra_deps: [ "//brave" ],
    is_component_build: this.buildConfig !== 'Release',
    proprietary_codecs: true,
    ffmpeg_branding: "Chrome",
    // branding_path_component: "brave",
    enable_widevine: process.platform !== 'linux',
    target_cpu: this.targetArch,
    is_official_build: this.officialBuild,
    is_debug: this.buildConfig !== 'Release',
    dcheck_always_on: this.buildConfig !== 'Release',
    brave_channel: this.channel,
     brave_google_api_key: this.googleApiKey,
     brave_google_api_endpoint: this.googleApiEndpoint,
     brave_product_name: getNPMConfig(['brave_product_name']) || "brave-core",
     brave_project_name: getNPMConfig(['brave_project_name']) || "brave-core",
     brave_version_major: version_parts[0],
     brave_version_minor: version_parts[1],
     brave_version_build: version_parts[2],
  }

  if (process.platform === 'darwin') {
    args.mac_signing_identifier = getNPMConfig(['mac_signing_identifier']) || ''
  }

  if (this.debugBuild) {
    if (process.platform === 'darwin') {
      args.enable_stripping = false
    }
    args.symbol_level = 2
    args.enable_profiling = true
    args.is_win_fastlink = true
  }

  if (this.targetArch === 'x86' && process.platform === 'linux') {
    // Minimal symbols for target Linux x86, because ELF32 cannot be > 4GiB
    args.symbol_level = 1
  }

  // TODO: Build redirect-cc.cmd in such a way that it still works with ccache if configured
  /*
  let sccache = getNPMConfig(['sccache'])
  if (sccache) {
    args.cc_wrapper = sccache
  }
  */

  if (process.platform === 'win32') {
    args.cc_wrapper = path.join(this.srcDir, 'brave', 'script' ,'redirect-cc.cmd')
  } else {
    args.cc_wrapper = path.join(this.srcDir, 'brave', 'script', 'redirect-cc.py')
  }
  return args
}

Config.prototype.prependPath = function (oldPath, addPath) {
  let newPath = oldPath.split(path.delimiter)
  newPath.unshift(addPath)
  newPath = newPath.join(path.delimiter)
  return newPath
}

Config.prototype.appendPath = function (oldPath, addPath) {
  let newPath = oldPath.split(path.delimiter)
  newPath.push(addPath)
  newPath = newPath.join(path.delimiter)
  return newPath
}

Config.prototype.addPathToEnv = function (env, addPath, prepend = false) {
  // cmd.exe uses Path instead of PATH so just set both
  const addToPath = prepend ? this.prependPath : this.appendPath
  env.Path && (env.Path = addToPath(env.Path, addPath))
  env.PATH && (env.PATH = addToPath(env.PATH, addPath))
  return env
}

Config.prototype.addPythonPathToEnv = function (env, addPath) {
  env.PYTHONPATH = this.appendPath(env.PYTHONPATH || '', addPath)
  return env
}

const getProjectVersion = function (projectName) {
  return getNPMConfig(['projects', projectName, 'tag']) || getNPMConfig(['projects', projectName, 'branch'])
}

Config.prototype.getProjectRef = function (projectName) {
  const ref = getNPMConfig(['projects', projectName, 'repository', 'ref'])
  if (ref) {
    return ref
  }

  const tag = getNPMConfig(['projects', projectName, 'tag'])
  if (tag) {
    return 'refs/tags/' + tag
  }

  let version = getNPMConfig(['projects', projectName, 'version'])
  let branch = getNPMConfig(['projects', projectName, 'branch'])
  if (!branch && !version) {
    return 'origin/master'
  }
  if (!version) {
    return `origin/${branch}`
  }
  branch = `origin/${version}`

  if (projectName === 'brave-core') {
    const chromeVersion = getProjectVersion('chrome')
    if (chromeVersion) {
      branch = `${branch}+${chromeVersion}`
    }
  }
  return branch
}

Config.prototype.buildProjects = function () {
  for (let name in packages.config.projects) {
    this.projectNames.push(name)
  }

  this.projectNames.forEach((projectName) => {
    this.projects[projectName] = {
      ref: this.getProjectRef(projectName),
      url: getNPMConfig(['projects', projectName, 'repository', 'url']),
      gclientName: getNPMConfig(['projects', projectName, 'dir']),
      dir: path.join(this.rootDir, getNPMConfig(['projects', projectName, 'dir'])),
      custom_deps: packages.config.projects[projectName].custom_deps
    }
  })
}

Config.prototype.update = function (options) {
  if (options.C) {
    this.buildConfig = path.basename(options.C)
    this.__outputDir = options.C
  }

  if (options.target_arch === 'x86') {
    this.targetArch = options.target_arch
    this.gypTargetArch = 'ia32'
  }

  if (options.target_arch === 'ia32') {
    this.targetArch = 'x86'
    this.gypTargetArch = options.target_arch
  }

  if (options.gclient_file && options.gclient_file !== 'default') {
    this.gClientFile = options.gclient_file
  }

  if (options.brave_google_api_key) {
    this.googleApiKey = options.brave_google_api_key
  }

  if (options.brave_google_api_endpoint) {
    this.googleApiEndpoint = options.brave_google_api_endpoint
  }

  if (options.debug_build !== null && options.debug_build !== undefined) {
    this.debugBuild = JSON.parse(options.debug_build)
  } else {
    this.debugBuild = this.buildConfig !== 'Release'
  }

  if (options.official_build !== null && options.official_build !== undefined) {
    this.officialBuild = JSON.parse(options.official_build)
    if (this.officialBuild) {
      this.debugBuild = false
    }
  } else {
    this.officialBuild = this.buildConfig === 'Release'
  }

  // In chromium src, empty string represents stable channel.
  if (options.channel !== 'release')
    this.channel = options.channel

  this.projectNames.forEach((projectName) => {
    // don't update refs for projects that have them
    if (!this.projects[projectName].ref)
      return

    let ref = options[projectName + '_ref']
    if (ref && ref !== 'default' && ref !== '') {
      this.projects[projectName].ref = ref
    }
  })
}

Object.defineProperty(Config.prototype, 'defaultOptions', {
  get: function () {
    let env = Object.assign({}, process.env)
    env = this.addPathToEnv(env, this.depotToolsDir, true)
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'brave', 'chromium_src', 'python_modules'))
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'brave', 'script'))
    env = this.addPythonPathToEnv(env, path.join(this.srcDir, 'tools', 'grit', 'grit', 'extern'))
    env.GCLIENT_FILE = this.gClientFile
    env.DEPOT_TOOLS_WIN_TOOLCHAIN = '0'
    env.PYTHONUNBUFFERED = '1'
    env.TARGET_ARCH = this.gypTargetArch // for brave scripts
    env.GYP_MSVS_VERSION = env.GYP_MSVS_VERSION || '2017' // enable 2017

    let sccache = getNPMConfig(['sccache'])
    if (sccache) {
      env.CC_WRAPPER = sccache
    }

    if (process.platform === 'linux') {
      env.LLVM_DOWNLOAD_GOLD_PLUGIN = '1'
    }

    return {
      env,
      stdio: 'inherit',
      cwd: this.srcDir,
      shell: true,
    }
  },
})

Object.defineProperty(Config.prototype, 'component', {
  get: function () { return this.__component || (this.buildConfig === 'Release' ? 'static_library' : 'shared_library') },
  set: function (component) { return this.__component = component },
})

Object.defineProperty(Config.prototype, 'outputDir', {
  get: function () {
    if (this.__outputDir)
      return this.__outputDir
    let baseDir = path.join(this.srcDir, 'out')
    baseDir = this.targetArch == 'x86' ? baseDir + '_x86' : baseDir
    return path.join(baseDir, this.buildConfig)
  },
  set: function (outputDir) { return this.__outputDir = outputDir },
})

module.exports = new Config
