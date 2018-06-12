const path = require('path')
const webpack = require('webpack')

module.exports = {
  devtool: '#inline-source-map',
  entry: {
    brave_new_tab: path.join(__dirname, '../brave_new_tab_ui/brave_new_tab'),
    brave_payments: path.join(__dirname, '../brave_payments_ui/brave_payments')
  },
  output: {
    path: path.join(__dirname, '..', '..', '..', process.env.TARGET_GEN_DIR, 'brave'),
    filename: '[name].bundle.js',
    chunkFilename: '[id].chunk.js'
  },
  plugins: [
    new webpack.optimize.OccurrenceOrderPlugin(),
    new webpack.IgnorePlugin(/[^/]+\/[\S]+.dev$/),
    new webpack.DefinePlugin({
      'process.env': {
        NODE_ENV: JSON.stringify('development')
      }
    })
  ],
  resolve: {
    extensions: ['.ts', '.tsx', '.js']
  },
  module: {
    rules: [
      {
        test: /\.tsx?$/,
        loader: 'awesome-typescript-loader'
      },
      {
        test: /\.js$/,
        loader: 'babel-loader',
        exclude: /node_modules/,
        query: {
          presets: ['react-optimize']
        }
      },
      {
        test: /\.less$/,
        loader: 'style-loader!css-loader?-minimize!less-loader'
      },
      {
        test: /\.css$/,
        loader: 'style-loader!css-loader?-minimize'
      },
      // Loads font files for Font Awesome
      {
        test: /\.woff(2)?(\?v=[0-9]\.[0-9]\.[0-9])?$/,
        loader: 'url-loader?limit=10000&minetype=application/font-woff'
      },
      {
        test: /\.(ttf|eot|svg|png|jpg)(\?v=[0-9]\.[0-9]\.[0-9])?$/,
        loader: 'file-loader'
      }]
  }
}
