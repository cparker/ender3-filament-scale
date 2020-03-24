const SerialPort = require('serialport')
const Readline = require('@serialport/parser-readline')
const axios = require('axios')
const _ = require('lodash')

const SLACK_WEBHOOK_URL = process.env.SLACK_WEBHOOK_URL
const OCTOPRINT_API_KEY = process.env.OCTOPRINT_API_KEY
const slack = require('slack-notify')(SLACK_WEBHOOK_URL)

const portName = process.env.PORT_NAME || '/dev/ttyUSB0'
const baudRate = process.env.BAUD_RATE || 9600
const port = new SerialPort(portName, { baudRate: baudRate })
const lowFilamentWarnLimitGrams = (process.env.WARN_GRAM_LIMIT = 150)
const lowFilamentCriticalLimitGrams = (process.env.CRITICAL_GRAM_LIMIT = 25)
const approxSpoolWeightG = process.env.SPOOL_WEIGHT || 250

let lastSendSlackMS = 0
const sendSlackMessageIntervalMin = 15

// this is the online and personally measured number for density of PLA filament
const PLAFilamentCMPerGram = 33.6

const parser = new Readline()
port.pipe(parser)

function sendSlack(message) {
  // slack.send({
  //   channel:'#ender',
  //   text:message
  // })
  console.log('sending slack ', message)
}

function getActivePrintStatus() {
  return axios.get('http://enderpi.local/api/job', {
    headers: {
      'x-api-key': OCTOPRINT_API_KEY
    }
  })
}


function checkSendMessage(messageText) {
  const nowMS = new Date().getTime()
  if (nowMS - lastSendSlackMS > sendSlackMessageIntervalMin * 60 * 1000) {
    lastSendSlackMS = nowMS
    sendSlack(messageText)
    console.log('sent slack message at ', new Date())
  }
}

function handleJobStatus(filamentRemainingG) {
  getActivePrintStatus()
    .then(response => {
      if (_.get(response, 'data.state', '') === 'Printing') {
        console.log('status: Printing')
        const jobFilLengthStr = _.get(
          response,
          'data.job.filament.tool0.length',
          ''
        )
        const jobFilLengthCM = parseFloat(jobFilLengthStr) / 10.0
        const jobFilWeightGrams = jobFilLengthCM / PLAFilamentCMPerGram
        console.log('current job filament weight is g ', jobFilWeightGrams)

        if (jobFilWeightGrams > filamentRemainingG) {
          checkSendMessage(`CAUTION: job requires ${jobFilWeightGrams} g but only ${filamentRemainingG} g remaining`)
        }

        if (filamentRemainingG < lowFilamentCriticalLimitGrams) {
          checkSendMessage(`CRITICAL: filament remaining is ${filamentRemainingG} grams`)
        } else if (lowFilamentWarnLimitGrams) {
          checkSendMessage(`WARN: filament reamining is ${filamentRemainingG} grams`)
        } else checkSendMessage(`OK: filament remaining ${filamentRemainingG} grams`)
      } else {
        // no active print job, so do nothing
      }
    })
    .catch(err => {
      console.error('caught error getting print status', err)
      checkSendMessage(`got error reading job status \n ${err}`)
    })
}

function handleSerialInput(input) {
  console.log('got ', input)

  // be sure we're handling the output of weight grams and not some other serial message
  if (input.lastIndexOf('weight grams:') !== -1) {
    const [_, weight] = input.split(':')
    const weightFloat = parseFloat(weight)
    const onlyFilamentWeight = weightFloat - approxSpoolWeightG

    if (onlyFilamentWeight < 0) {
      console.log(`filament remaining seems invalid ${onlyFilamentWeight}`)
      return
    }

    handleJobStatus(onlyFilamentWeight)
  }
}

parser.on('data', handleSerialInput)
