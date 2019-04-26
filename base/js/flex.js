'use strict'

global.tokenize = function (query, type) {
  const result = []
  if (!flex_tokenize) {
    return result
  }
  const arr = flex_tokenize(query, type) || []
  for (let i = 0; i < arr.length; i += 2) {
    let start = arr[i]
    let stop = arr[i + 1]
    result.push({
      start,
      stop,
      text: query.substring(start, stop)
    })
  }
  return result
}